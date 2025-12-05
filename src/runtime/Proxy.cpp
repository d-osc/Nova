// Proxy.cpp - ES2015 Proxy implementation for Nova
// Provides metaprogramming capabilities through handler traps

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>

extern "C" {

// Forward declarations from other runtime files
void* nova_object_create_empty();
void nova_object_set(void* obj, const char* key, void* value);
void* nova_object_get(void* obj, const char* key);
int64_t nova_object_has(void* obj, const char* key);
int64_t nova_object_delete(void* obj, const char* key);
void* nova_object_keys(void* obj);
void* nova_value_array_create();
void nova_value_array_push(void* arr, void* value);
int64_t nova_value_array_length(void* arr);
void* nova_value_array_at(void* arr, int64_t index);
const char* nova_value_to_string(void* value);
void* nova_create_string(const char* str);

// Handler trap function pointer types
typedef void* (*TrapGet)(void* target, const char* prop, void* receiver);
typedef int64_t (*TrapSet)(void* target, const char* prop, void* value, void* receiver);
typedef int64_t (*TrapHas)(void* target, const char* prop);
typedef int64_t (*TrapDeleteProperty)(void* target, const char* prop);
typedef void* (*TrapOwnKeys)(void* target);
typedef void* (*TrapGetOwnPropertyDescriptor)(void* target, const char* prop);
typedef int64_t (*TrapDefineProperty)(void* target, const char* prop, void* descriptor);
typedef int64_t (*TrapPreventExtensions)(void* target);
typedef void* (*TrapGetPrototypeOf)(void* target);
typedef int64_t (*TrapSetPrototypeOf)(void* target, void* proto);
typedef int64_t (*TrapIsExtensible)(void* target);
typedef void* (*TrapApply)(void* target, void* thisArg, void* args);
typedef void* (*TrapConstruct)(void* target, void* args, void* newTarget);

// Proxy structure
struct NovaProxy {
    void* target;           // The target object being proxied
    void* handler;          // The handler object with traps
    bool revoked;           // Whether the proxy has been revoked

    // Cached trap functions (resolved from handler)
    void* trap_get;
    void* trap_set;
    void* trap_has;
    void* trap_deleteProperty;
    void* trap_ownKeys;
    void* trap_getOwnPropertyDescriptor;
    void* trap_defineProperty;
    void* trap_preventExtensions;
    void* trap_getPrototypeOf;
    void* trap_setPrototypeOf;
    void* trap_isExtensible;
    void* trap_apply;
    void* trap_construct;
};

// Revocable proxy result structure
struct RevocableProxy {
    void* proxy;
    void* revoke;  // Function to revoke the proxy
};

// Global storage for revocable proxies
static std::vector<NovaProxy*> revocableProxies;

// Create a new Proxy
void* nova_proxy_create(void* target, void* handler) {
    NovaProxy* proxy = new NovaProxy();
    proxy->target = target;
    proxy->handler = handler;
    proxy->revoked = false;

    // Cache trap references from handler
    if (handler) {
        proxy->trap_get = nova_object_get(handler, "get");
        proxy->trap_set = nova_object_get(handler, "set");
        proxy->trap_has = nova_object_get(handler, "has");
        proxy->trap_deleteProperty = nova_object_get(handler, "deleteProperty");
        proxy->trap_ownKeys = nova_object_get(handler, "ownKeys");
        proxy->trap_getOwnPropertyDescriptor = nova_object_get(handler, "getOwnPropertyDescriptor");
        proxy->trap_defineProperty = nova_object_get(handler, "defineProperty");
        proxy->trap_preventExtensions = nova_object_get(handler, "preventExtensions");
        proxy->trap_getPrototypeOf = nova_object_get(handler, "getPrototypeOf");
        proxy->trap_setPrototypeOf = nova_object_get(handler, "setPrototypeOf");
        proxy->trap_isExtensible = nova_object_get(handler, "isExtensible");
        proxy->trap_apply = nova_object_get(handler, "apply");
        proxy->trap_construct = nova_object_get(handler, "construct");
    } else {
        proxy->trap_get = nullptr;
        proxy->trap_set = nullptr;
        proxy->trap_has = nullptr;
        proxy->trap_deleteProperty = nullptr;
        proxy->trap_ownKeys = nullptr;
        proxy->trap_getOwnPropertyDescriptor = nullptr;
        proxy->trap_defineProperty = nullptr;
        proxy->trap_preventExtensions = nullptr;
        proxy->trap_getPrototypeOf = nullptr;
        proxy->trap_setPrototypeOf = nullptr;
        proxy->trap_isExtensible = nullptr;
        proxy->trap_apply = nullptr;
        proxy->trap_construct = nullptr;
    }

    return proxy;
}

// Revoke function for revocable proxies
void nova_proxy_revoke_internal(void* proxy_ptr) {
    if (!proxy_ptr) return;
    NovaProxy* proxy = static_cast<NovaProxy*>(proxy_ptr);
    proxy->revoked = true;
}

// Create a revocable Proxy - returns { proxy, revoke }
void* nova_proxy_revocable(void* target, void* handler) {
    NovaProxy* proxy = static_cast<NovaProxy*>(nova_proxy_create(target, handler));
    revocableProxies.push_back(proxy);

    // Create result object with proxy and revoke function
    void* result = nova_object_create_empty();
    nova_object_set(result, "proxy", proxy);

    // Store proxy index for revoke function
    // In a real implementation, revoke would be a closure
    // For now, we store the proxy pointer directly
    nova_object_set(result, "revoke", proxy);

    return result;
}

// Check if proxy is revoked
int64_t nova_proxy_is_revoked(void* proxy_ptr) {
    if (!proxy_ptr) return 1;
    NovaProxy* proxy = static_cast<NovaProxy*>(proxy_ptr);
    return proxy->revoked ? 1 : 0;
}

// Get the proxy target
void* nova_proxy_get_target(void* proxy_ptr) {
    if (!proxy_ptr) return nullptr;
    NovaProxy* proxy = static_cast<NovaProxy*>(proxy_ptr);
    if (proxy->revoked) {
        fprintf(stderr, "TypeError: Cannot perform operation on a revoked proxy\n");
        return nullptr;
    }
    return proxy->target;
}

// Get the proxy handler
void* nova_proxy_get_handler(void* proxy_ptr) {
    if (!proxy_ptr) return nullptr;
    NovaProxy* proxy = static_cast<NovaProxy*>(proxy_ptr);
    if (proxy->revoked) {
        fprintf(stderr, "TypeError: Cannot perform operation on a revoked proxy\n");
        return nullptr;
    }
    return proxy->handler;
}

// ============== Trap Implementations ==============

// get trap - intercepts property access
void* nova_proxy_trap_get(void* proxy_ptr, const char* prop, [[maybe_unused]] void* receiver) {
    if (!proxy_ptr) return nullptr;
    NovaProxy* proxy = static_cast<NovaProxy*>(proxy_ptr);

    if (proxy->revoked) {
        fprintf(stderr, "TypeError: Cannot perform 'get' on a revoked proxy\n");
        return nullptr;
    }

    // If handler has get trap, call it
    if (proxy->trap_get) {
        // In full implementation, would call the trap function
        // For now, fall through to target
    }

    // Default behavior: get from target
    return nova_object_get(proxy->target, prop);
}

// set trap - intercepts property assignment
int64_t nova_proxy_trap_set(void* proxy_ptr, const char* prop, void* value, [[maybe_unused]] void* receiver) {
    if (!proxy_ptr) return 0;
    NovaProxy* proxy = static_cast<NovaProxy*>(proxy_ptr);

    if (proxy->revoked) {
        fprintf(stderr, "TypeError: Cannot perform 'set' on a revoked proxy\n");
        return 0;
    }

    // If handler has set trap, call it
    if (proxy->trap_set) {
        // In full implementation, would call the trap function
        // For now, fall through to target
    }

    // Default behavior: set on target
    nova_object_set(proxy->target, prop, value);
    return 1;
}

// has trap - intercepts 'in' operator
int64_t nova_proxy_trap_has(void* proxy_ptr, const char* prop) {
    if (!proxy_ptr) return 0;
    NovaProxy* proxy = static_cast<NovaProxy*>(proxy_ptr);

    if (proxy->revoked) {
        fprintf(stderr, "TypeError: Cannot perform 'has' on a revoked proxy\n");
        return 0;
    }

    // If handler has 'has' trap, call it
    if (proxy->trap_has) {
        // In full implementation, would call the trap function
    }

    // Default behavior: check target
    return nova_object_has(proxy->target, prop);
}

// deleteProperty trap - intercepts delete operator
int64_t nova_proxy_trap_deleteProperty(void* proxy_ptr, const char* prop) {
    if (!proxy_ptr) return 0;
    NovaProxy* proxy = static_cast<NovaProxy*>(proxy_ptr);

    if (proxy->revoked) {
        fprintf(stderr, "TypeError: Cannot perform 'deleteProperty' on a revoked proxy\n");
        return 0;
    }

    // If handler has deleteProperty trap, call it
    if (proxy->trap_deleteProperty) {
        // In full implementation, would call the trap function
    }

    // Default behavior: delete from target
    return nova_object_delete(proxy->target, prop);
}

// ownKeys trap - intercepts Object.keys, Object.getOwnPropertyNames, etc.
void* nova_proxy_trap_ownKeys(void* proxy_ptr) {
    if (!proxy_ptr) return nova_value_array_create();
    NovaProxy* proxy = static_cast<NovaProxy*>(proxy_ptr);

    if (proxy->revoked) {
        fprintf(stderr, "TypeError: Cannot perform 'ownKeys' on a revoked proxy\n");
        return nova_value_array_create();
    }

    // If handler has ownKeys trap, call it
    if (proxy->trap_ownKeys) {
        // In full implementation, would call the trap function
    }

    // Default behavior: get keys from target
    return nova_object_keys(proxy->target);
}

// getOwnPropertyDescriptor trap
void* nova_proxy_trap_getOwnPropertyDescriptor(void* proxy_ptr, const char* prop) {
    if (!proxy_ptr) return nullptr;
    NovaProxy* proxy = static_cast<NovaProxy*>(proxy_ptr);

    if (proxy->revoked) {
        fprintf(stderr, "TypeError: Cannot perform 'getOwnPropertyDescriptor' on a revoked proxy\n");
        return nullptr;
    }

    // If handler has trap, call it
    if (proxy->trap_getOwnPropertyDescriptor) {
        // In full implementation, would call the trap function
    }

    // Default behavior: create basic descriptor if property exists
    void* value = nova_object_get(proxy->target, prop);
    if (value) {
        void* descriptor = nova_object_create_empty();
        nova_object_set(descriptor, "value", value);
        nova_object_set(descriptor, "writable", reinterpret_cast<void*>(1));
        nova_object_set(descriptor, "enumerable", reinterpret_cast<void*>(1));
        nova_object_set(descriptor, "configurable", reinterpret_cast<void*>(1));
        return descriptor;
    }
    return nullptr;
}

// defineProperty trap
int64_t nova_proxy_trap_defineProperty(void* proxy_ptr, const char* prop, void* descriptor) {
    if (!proxy_ptr) return 0;
    NovaProxy* proxy = static_cast<NovaProxy*>(proxy_ptr);

    if (proxy->revoked) {
        fprintf(stderr, "TypeError: Cannot perform 'defineProperty' on a revoked proxy\n");
        return 0;
    }

    // If handler has trap, call it
    if (proxy->trap_defineProperty) {
        // In full implementation, would call the trap function
    }

    // Default behavior: define on target
    if (descriptor) {
        void* value = nova_object_get(descriptor, "value");
        if (value) {
            nova_object_set(proxy->target, prop, value);
        }
    }
    return 1;
}

// preventExtensions trap
int64_t nova_proxy_trap_preventExtensions(void* proxy_ptr) {
    if (!proxy_ptr) return 0;
    NovaProxy* proxy = static_cast<NovaProxy*>(proxy_ptr);

    if (proxy->revoked) {
        fprintf(stderr, "TypeError: Cannot perform 'preventExtensions' on a revoked proxy\n");
        return 0;
    }

    // If handler has trap, call it
    if (proxy->trap_preventExtensions) {
        // In full implementation, would call the trap function
    }

    // Default behavior would prevent extensions on target
    return 1;
}

// getPrototypeOf trap
void* nova_proxy_trap_getPrototypeOf(void* proxy_ptr) {
    if (!proxy_ptr) return nullptr;
    NovaProxy* proxy = static_cast<NovaProxy*>(proxy_ptr);

    if (proxy->revoked) {
        fprintf(stderr, "TypeError: Cannot perform 'getPrototypeOf' on a revoked proxy\n");
        return nullptr;
    }

    // If handler has trap, call it
    if (proxy->trap_getPrototypeOf) {
        // In full implementation, would call the trap function
    }

    // Default: return null (Object.prototype would be the actual default)
    return nullptr;
}

// setPrototypeOf trap
int64_t nova_proxy_trap_setPrototypeOf(void* proxy_ptr, [[maybe_unused]] void* proto) {
    if (!proxy_ptr) return 0;
    NovaProxy* proxy = static_cast<NovaProxy*>(proxy_ptr);

    if (proxy->revoked) {
        fprintf(stderr, "TypeError: Cannot perform 'setPrototypeOf' on a revoked proxy\n");
        return 0;
    }

    // If handler has trap, call it
    if (proxy->trap_setPrototypeOf) {
        // In full implementation, would call the trap function
    }

    // Default behavior
    return 1;
}

// isExtensible trap
int64_t nova_proxy_trap_isExtensible(void* proxy_ptr) {
    if (!proxy_ptr) return 0;
    NovaProxy* proxy = static_cast<NovaProxy*>(proxy_ptr);

    if (proxy->revoked) {
        fprintf(stderr, "TypeError: Cannot perform 'isExtensible' on a revoked proxy\n");
        return 0;
    }

    // If handler has trap, call it
    if (proxy->trap_isExtensible) {
        // In full implementation, would call the trap function
    }

    // Default: return true (objects are extensible by default)
    return 1;
}

// apply trap - intercepts function calls
void* nova_proxy_trap_apply(void* proxy_ptr, [[maybe_unused]] void* thisArg, [[maybe_unused]] void* args) {
    if (!proxy_ptr) return nullptr;
    NovaProxy* proxy = static_cast<NovaProxy*>(proxy_ptr);

    if (proxy->revoked) {
        fprintf(stderr, "TypeError: Cannot perform 'apply' on a revoked proxy\n");
        return nullptr;
    }

    // If handler has apply trap, call it
    if (proxy->trap_apply) {
        // In full implementation, would call the trap function
    }

    // Default behavior would call the target function
    // This requires function call infrastructure
    return nullptr;
}

// construct trap - intercepts new operator
void* nova_proxy_trap_construct(void* proxy_ptr, [[maybe_unused]] void* args, [[maybe_unused]] void* newTarget) {
    if (!proxy_ptr) return nullptr;
    NovaProxy* proxy = static_cast<NovaProxy*>(proxy_ptr);

    if (proxy->revoked) {
        fprintf(stderr, "TypeError: Cannot perform 'construct' on a revoked proxy\n");
        return nullptr;
    }

    // If handler has construct trap, call it
    if (proxy->trap_construct) {
        // In full implementation, would call the trap function
    }

    // Default behavior would construct using target
    return nova_object_create_empty();
}

// Revoke a proxy (public API)
void nova_proxy_revoke(void* proxy_ptr) {
    nova_proxy_revoke_internal(proxy_ptr);
}

} // extern "C"
