// Proxy.cpp - ES2015 Proxy implementation for Nova
// Provides metaprogramming capabilities through handler traps.
//
// Phase 2.4 implementation: instead of returning a separate NovaProxy
// struct, nova_proxy_create now returns a regular runtime Object whose
// identity is registered in a global side-table. Property-access runtime
// entry points (nova_dynamic_object_get_tagged / _set_tagged /
// nova_object_has / nova_object_delete) consult that side-table and, when
// the operand is a registered proxy, dispatch to the handler's trap
// function (a stored callable, see nova_dynamic_object_set_function).
//
// Trap handlers are JS methods on the handler Object. They are compiled
// to free C functions with signature
//   int64_t (*)(int64_t this, int64_t arg1, int64_t arg2, ...)
// (the runtime-object-method convention from HIRGen_Objects.cpp). All
// arguments are NaN-boxed JSValues. On Windows x64 the calling convention
// places every 8-byte argument in a register (RCX, RDX, R8, R9) or on the
// stack, so a function pointer of any arity is callable as long as we
// pass the right number of i64 words.

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <unordered_map>

#include "nova/runtime/Value.h"

extern "C" {

// Forward declarations from other runtime files
void* nova_object_create_empty();
void nova_object_set(void* obj, const char* key, void* value);
void* nova_object_get(void* obj, const char* key);
int64_t nova_object_has(void* obj, const char* key);
int64_t nova_object_delete(void* obj, const char* key);
void* nova_object_keys(void* obj);
void* nova_value_array_create(int64_t length);

// Runtime helpers from Object.cpp
void nova_dynamic_object_set_function(void* obj, const char* key, void* fnPtr);
void* nova_dynamic_object_get_function(void* obj, const char* key);
void nova_dynamic_object_set_function_with_env(void* obj, const char* key,
                                                void* fnPtr, void* envPtr);
void* nova_dynamic_object_get_function_env(void* obj, const char* key);

std::uint64_t nova_value_from_object(void* value);
std::uint64_t nova_value_from_string(const char* value);
std::uint64_t nova_value_from_i64(std::int64_t value);
void* nova_value_to_object(std::uint64_t value);

// nova_throw_type_error sets the global exception-pending flag (and stores
// the error pointer in g_exception_value). Used by trap dispatch when a
// revoked proxy is touched inside a try block.
void nova_throw_type_error(const char* message);

// ============================================================================
// Proxy registry — side-table mapping runtime Object* → ProxyInfo.
// A runtime Object is a Proxy iff it appears in this table.
// ============================================================================
struct ProxyInfo {
    void* target;
    void* handler;
    bool revoked;
};

static std::unordered_map<void*, ProxyInfo*>& proxy_registry() {
    static std::unordered_map<void*, ProxyInfo*> registry;
    return registry;
}

// Public accessor used by Object.cpp property-access hooks.
// Returns the ProxyInfo* (cast to void*) if obj is a registered proxy,
// or nullptr otherwise. Returns void* to keep Object.cpp's forward
// declaration simple (no header dependency on ProxyInfo).
void* nova_proxy_get_info(void* obj) {
    if (!obj) return nullptr;
    auto& reg = proxy_registry();
    auto it = reg.find(obj);
    return it != reg.end() ? static_cast<void*>(it->second) : nullptr;
}

// Internal helper for use inside Proxy.cpp where the ProxyInfo* is needed.
static ProxyInfo* get_info(void* obj) {
    return static_cast<ProxyInfo*>(nova_proxy_get_info(obj));
}

// ============================================================================
// nova_proxy_create(target, handler) — returns a runtime Object* registered
// as a proxy. Subsequent property accesses via nova_dynamic_object_get_tagged
// / _set_tagged / nova_object_has / nova_object_delete will dispatch through
// the handler's traps (if present) instead of touching the target directly.
// ============================================================================
void* nova_proxy_create(void* target, void* handler) {
    void* proxy = nova_object_create_empty();
    ProxyInfo* info = new ProxyInfo{target, handler, false};
    proxy_registry()[proxy] = info;
    return proxy;
}

// ============================================================================
// Proxy.revocable(target, handler) — returns { proxy, revoke }.
// We materialize the result as a runtime Object. The "revoke" property
// holds a small Callable that, when invoked, flips the revoked flag.
// The revoke callable receives the result Object itself as its first
// argument (the calling convention for stored functions), then looks up
// the stashed __revoke_target__ to find the proxy to revoke.
// ============================================================================
void nova_proxy_revoke_by_proxy(void* proxy_ptr);
int64_t nova_proxy_revoke_callable(void* this_arg) {
    if (!this_arg) return 0;
    void* proxy = nova_object_get(this_arg, "__revoke_target__");
    if (proxy) nova_proxy_revoke_by_proxy(proxy);
    return 0;
}

void* nova_proxy_revocable(void* target, void* handler) {
    void* proxy = nova_proxy_create(target, handler);
    void* result = nova_object_create_empty();
    // Store the proxy under "proxy" as an OBJECT-tagged JSValue so that
    // reads via nova_dynamic_object_get_tagged + nova_value_to_object
    // round-trip correctly.
    nova_object_set(result, "proxy",
        reinterpret_cast<void*>(static_cast<uintptr_t>(nova_value_from_object(proxy))));
    // Store the revoke callable under "revoke" similarly.
    void* revokeFn = reinterpret_cast<void*>(&nova_proxy_revoke_callable);
    nova_dynamic_object_set_function(result, "revoke", revokeFn);
    // Stash proxy pointer on the result so revoke_callable can find it
    // by reading __revoke_target__ at invocation time.
    nova_object_set(result, "__revoke_target__", proxy);
    return result;
}

void nova_proxy_revoke_by_proxy(void* proxy_ptr) {
    ProxyInfo* info = get_info(proxy_ptr);
    if (info) info->revoked = true;
}

// Revoke a proxy given the result Object of nova_proxy_revocable.
void nova_proxy_revoke(void* result_obj) {
    if (!result_obj) return;
    void* proxy = nova_object_get(result_obj, "__revoke_target__");
    if (!proxy) return;
    // proxy was stored raw (untagged) via nova_object_set above; cast through.
    nova_proxy_revoke_by_proxy(proxy);
}

int64_t nova_proxy_is_revoked(void* proxy_ptr) {
    ProxyInfo* info = get_info(proxy_ptr);
    return (info && info->revoked) ? 1 : 0;
}

void* nova_proxy_get_target(void* proxy_ptr) {
    ProxyInfo* info = get_info(proxy_ptr);
    if (!info || info->revoked) return nullptr;
    return info->target;
}

void* nova_proxy_get_handler(void* proxy_ptr) {
    ProxyInfo* info = get_info(proxy_ptr);
    if (!info || info->revoked) return nullptr;
    return info->handler;
}

// ============================================================================
// Trap dispatch entry points.
//
// Each trap returns:
//  - For get/ownKeys/getPrototypeOf/apply/construct: a value (i64 JSValue
//    or ptr, depending on the trap).
//  - For set/has/deleteProperty/defineProperty/preventExtensions/
//    setPrototypeOf/isExtensible: a boolean (int64_t).
//
// We invoke the handler's stored callable via a C function pointer of the
// appropriate arity. JSValue-tagged arguments are constructed from the
// raw C-side values (target ptr → OBJECT JSValue, prop const char* →
// STRING JSValue, etc.).
// ============================================================================

// Helper: throw a TypeError when a revoked proxy is used. The runtime
// g_exception_pending flag is set by nova_throw_type_error → nova_throw,
// then the HIR-level exception poll after nova_dynamic_object_get_tagged
// transfers control to the enclosing catch block. If no try is active,
// nova_throw exits the process with the uncaught message.
static void proxy_type_error(const char* op) {
    char buf[160];
    std::snprintf(buf, sizeof(buf),
        "Cannot perform '%s' on a proxy that has been revoked", op);
    nova_throw_type_error(buf);
}

// Trap signatures — match the runtime-object-method calling convention.
// FunctionExpr strips an unused __this parameter, so for trap methods that
// don't reference `this` in their body, the visible signature is just
// (userArgs..., env). The trailing void* is the closure-environment pointer
// (null if the method captures nothing). If a trap body ever uses `this`,
// FunctionExpr keeps __this as the leading arg — that case is not currently
// exercised by js_spec_proxy_reflect.js.
using TrapGetFn    = int64_t (*)(int64_t, int64_t, int64_t, void*);
using TrapSetFn    = int64_t (*)(int64_t, int64_t, int64_t, int64_t, void*);
using TrapHasFn    = int64_t (*)(int64_t, int64_t, void*);
using TrapDeleteFn = int64_t (*)(int64_t, int64_t, void*);
using TrapApplyFn  = int64_t (*)(int64_t, int64_t, int64_t, int64_t, void*);
using TrapConstructFn = int64_t (*)(int64_t, int64_t, int64_t, int64_t, void*);
using TrapBool1Fn  = int64_t (*)(int64_t, int64_t, void*);
using TrapBool2Fn  = int64_t (*)(int64_t, int64_t, int64_t, void*);

// ============================================================================
// Get trap — invoked by nova_dynamic_object_get_tagged when the operand is
// a registered proxy. Returns the i64 JSValue the handler produced.
// ============================================================================
std::uint64_t nova_proxy_dispatch_get(void* proxy_ptr, const char* prop,
                                       void* receiver) {
    ProxyInfo* info = get_info(proxy_ptr);
    if (!info) return 0;
    if (info->revoked) { proxy_type_error("get"); return 0; }

    void* trapFn = nova_dynamic_object_get_function(info->handler, "get");
    int64_t targetJs  = static_cast<int64_t>(nova_value_from_object(info->target));
    int64_t keyJs     = static_cast<int64_t>(nova_value_from_string(prop ? prop : ""));
    int64_t receiverJs = static_cast<int64_t>(nova_value_from_object(
        receiver ? receiver : proxy_ptr));

    if (trapFn) {
        void* env = nova_dynamic_object_get_function_env(info->handler, "get");
        TrapGetFn fn = reinterpret_cast<TrapGetFn>(trapFn);
        return static_cast<std::uint64_t>(
            fn(targetJs, keyJs, receiverJs, env));
    }
    // Default: read directly from target.
    void* val = nova_object_get(info->target, prop);
    if (!val) return 0;
    return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(val));
}

// ============================================================================
// Set trap — invoked by nova_dynamic_object_set_tagged.
// ============================================================================
int64_t nova_proxy_dispatch_set(void* proxy_ptr, const char* prop,
                                 std::uint64_t valueJs, void* receiver) {
    ProxyInfo* info = get_info(proxy_ptr);
    if (!info) return 0;
    if (info->revoked) { proxy_type_error("set"); return 0; }

    void* trapFn = nova_dynamic_object_get_function(info->handler, "set");
    int64_t targetJs  = static_cast<int64_t>(nova_value_from_object(info->target));
    int64_t keyJs     = static_cast<int64_t>(nova_value_from_string(prop ? prop : ""));
    int64_t valueArg  = static_cast<int64_t>(valueJs);
    int64_t receiverJs = static_cast<int64_t>(nova_value_from_object(
        receiver ? receiver : proxy_ptr));

    if (trapFn) {
        void* env = nova_dynamic_object_get_function_env(info->handler, "set");
        TrapSetFn fn = reinterpret_cast<TrapSetFn>(trapFn);
        return fn(targetJs, keyJs, valueArg, receiverJs, env);
    }
    // Default: set on target. Store as a tagged JSValue so a later read
    // via nova_dynamic_object_get_tagged returns the same bits.
    nova_object_set(info->target, prop,
        reinterpret_cast<void*>(static_cast<std::uintptr_t>(valueJs)));
    return 1;
}

// ============================================================================
// Has trap — invoked by nova_object_has.
// ============================================================================
int64_t nova_proxy_dispatch_has(void* proxy_ptr, const char* prop) {
    ProxyInfo* info = get_info(proxy_ptr);
    if (!info) return 0;
    if (info->revoked) { proxy_type_error("has"); return 0; }

    void* trapFn = nova_dynamic_object_get_function(info->handler, "has");
    int64_t targetJs  = static_cast<int64_t>(nova_value_from_object(info->target));
    int64_t keyJs     = static_cast<int64_t>(nova_value_from_string(prop ? prop : ""));

    if (trapFn) {
        void* env = nova_dynamic_object_get_function_env(info->handler, "has");
        TrapHasFn fn = reinterpret_cast<TrapHasFn>(trapFn);
        return fn(targetJs, keyJs, env);
    }
    return nova_object_has(info->target, prop);
}

// ============================================================================
// Delete trap — invoked by nova_object_delete.
// ============================================================================
int64_t nova_proxy_dispatch_delete(void* proxy_ptr, const char* prop) {
    ProxyInfo* info = get_info(proxy_ptr);
    if (!info) return 0;
    if (info->revoked) { proxy_type_error("deleteProperty"); return 0; }

    void* trapFn = nova_dynamic_object_get_function(info->handler, "deleteProperty");
    int64_t targetJs  = static_cast<int64_t>(nova_value_from_object(info->target));
    int64_t keyJs     = static_cast<int64_t>(nova_value_from_string(prop ? prop : ""));

    if (trapFn) {
        void* env = nova_dynamic_object_get_function_env(info->handler, "deleteProperty");
        TrapDeleteFn fn = reinterpret_cast<TrapDeleteFn>(trapFn);
        return fn(targetJs, keyJs, env);
    }
    return nova_object_delete(info->target, prop);
}

// Apply / construct traps are not currently dispatched from the HIR call
// path — they require the unified Callable ABI to fully land. Stubs are
// kept here so proxy_reflect's `Reflect.apply` assertion (line 35) can
// call into runtime via a thin wrapper when ready.
void* nova_proxy_trap_apply(void* proxy_ptr, void* thisArg, void* args) {
    (void)proxy_ptr; (void)thisArg; (void)args;
    return nullptr;
}

void* nova_proxy_trap_construct(void* proxy_ptr, void* args, void* newTarget) {
    (void)proxy_ptr; (void)args; (void)newTarget;
    return nova_object_create_empty();
}

} // extern "C"
