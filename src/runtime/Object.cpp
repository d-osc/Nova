#include "nova/runtime/Runtime.h"
#include "nova/runtime/Value.h"
#include <cmath>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <string>

namespace nova {
namespace runtime {

struct Property {
    void* value;
    TypeId type_id;
    // Lower 3 bits hold descriptor flags (writable/enumerable/configurable).
    // Defaults to writable|enumerable|configurable (PROP_DEFAULT_FLAGS = 7).
    uint32_t flags;
};

// Property storage with insertion order tracking.
// Stored as a void* in Object::properties; created lazily.
//
// String keys and symbol keys live in parallel containers so the spec's
// own-key ordering (integer-index strings ascending, then string keys in
// insertion order, then symbol keys in insertion order) can be honored.
// We don't model integer-index strings distinctly yet — they're stored as
// ordinary string keys, in violation of the spec's "[[OwnPropertyKeys]]
// integer indices first" rule. That's tracked separately.
struct PropertyStorage {
    // String-keyed properties.
    std::unordered_map<std::string, size_t> lookup;
    struct Entry { std::string key; Property prop; };
    std::vector<Entry> entries;

    // Symbol-keyed properties. The key is the NovaSymbol pointer (stable
    // identity). Tracked separately so a Symbol round-trips through
    // property access without losing identity.
    std::unordered_map<void*, size_t> symbolLookup;
    struct SymbolEntry { void* symbol; Property prop; };
    std::vector<SymbolEntry> symbolEntries;
};

static PropertyStorage* get_storage(Object* obj) {
    if (!obj->properties) {
        obj->properties = new PropertyStorage();
    }
    return static_cast<PropertyStorage*>(obj->properties);
}

Object* create_object() {
    // Allocate object structure
    Object* obj = static_cast<Object*>(allocate(sizeof(Object), TypeId::OBJECT));

    // Initialize object
    obj->properties = nullptr; // Will be lazily allocated
    obj->proto = nullptr;      // No prototype by default; set by create(proto)/Error constructors
    obj->integrity = 0;        // 0 = normal/extensible, 1 = sealed, 2 = frozen, 3 = non-extensible

    return obj;
}

void* object_get(Object* obj, const char* key) {
    if (!obj || !key) return nullptr;

    // Walk the prototype chain. We treat ObjectHeader::next as the GC next-pointer,
    // and Object::proto as the [[Prototype]] link.
    Object* current = obj;
    while (current) {
        if (current->properties) {
            auto* storage = static_cast<PropertyStorage*>(current->properties);
            auto it = storage->lookup.find(key);
            if (it != storage->lookup.end()) {
                return storage->entries[it->second].prop.value;
            }
        }
        current = static_cast<Object*>(current->proto);
    }

    return nullptr;
}

void object_set(Object* obj, const char* key, void* value) {
    if (!obj || !key) return;

    PropertyStorage* storage = obj->properties
        ? static_cast<PropertyStorage*>(obj->properties)
        : nullptr;

    // If non-extensible, refuse new keys. Existing writable keys still update.
    // (sealed: no add/delete, writable bits respected; frozen: no edit at all)
    if (obj->integrity == 3 /* non-extensible */ ||
        obj->integrity == 1 /* sealed */ ||
        obj->integrity == 2 /* frozen */) {
        if (storage) {
            auto it = storage->lookup.find(key);
            if (it == storage->lookup.end()) {
                return; // can't add
            }
            if (obj->integrity == 2 /* frozen */) {
                return; // can't edit value
            }
            if (!(storage->entries[it->second].prop.flags & PROP_WRITABLE)) {
                return; // writable=false
            }
        } else {
            return; // can't add
        }
    }

    if (!storage) {
        storage = get_storage(obj);
    }

    Property prop;
    prop.value = value;
    prop.type_id = TypeId::OBJECT; // Default type
    prop.flags = PROP_DEFAULT_FLAGS;

    auto existing = storage->lookup.find(key);
    if (existing != storage->lookup.end()) {
        // Even on extensible objects, respect the writable bit. Spec
        // semantics: setting a non-writable data property is a silent
        // no-op in non-strict mode.
        if (!(storage->entries[existing->second].prop.flags & PROP_WRITABLE)) {
            return;
        }
        // Preserve flags of existing key.
        prop.flags = storage->entries[existing->second].prop.flags;
        storage->entries[existing->second].prop = prop;
    } else {
        storage->lookup[key] = storage->entries.size();
        storage->entries.push_back({std::string(key), prop});
    }
}

bool object_has(Object* obj, const char* key) {
    if (!obj || !key) return false;

    Object* current = obj;
    while (current) {
        if (current->properties) {
            auto* storage = static_cast<PropertyStorage*>(current->properties);
            if (storage->lookup.find(key) != storage->lookup.end()) {
                return true;
            }
        }
        current = static_cast<Object*>(current->proto);
    }
    return false;
}

void object_delete(Object* obj, const char* key) {
    if (!obj || !key) return;

    // Sealed/frozen/non-extensible objects disallow deletion.
    if (obj->integrity != 0) {
        return;
    }

    if (!obj->properties) {
        return;
    }

    auto* storage = static_cast<PropertyStorage*>(obj->properties);
    auto it = storage->lookup.find(key);
    if (it == storage->lookup.end()) {
        return;
    }
    if (!(storage->entries[it->second].prop.flags & PROP_CONFIGURABLE)) {
        return; // configurable=false, cannot delete
    }

    size_t idx = it->second;
    storage->entries.erase(storage->entries.begin() + idx);
    storage->lookup.erase(it);
    // Rebuild lookup indices after erase.
    for (size_t i = idx; i < storage->entries.size(); ++i) {
        storage->lookup[storage->entries[i].key] = i;
    }

    // Clean up if empty
    if (storage->entries.empty()) {
        delete storage;
        obj->properties = nullptr;
    }
}

// Internal helper: defineProperty implementation operating on raw flags.
// Used by both Object.defineProperty and the descriptor-aware object_set.
bool object_define_own(Object* obj, const char* key, void* value,
                       uint32_t flags) {
    if (!obj || !key) return false;

    PropertyStorage* storage = obj->properties
        ? static_cast<PropertyStorage*>(obj->properties)
        : nullptr;

    // If non-extensible and key is new, refuse. (sealed/frozen also block adds.)
    if (obj->integrity != 0) {
        if (storage) {
            if (storage->lookup.find(key) == storage->lookup.end()) {
                return false; // can't add new key on integrity-locked object
            }
        } else {
            return false;
        }
    }

    if (!storage) {
        storage = get_storage(obj);
    }

    auto existing = storage->lookup.find(key);
    if (existing != storage->lookup.end()) {
        Property& existingProp = storage->entries[existing->second].prop;
        if (!(existingProp.flags & PROP_CONFIGURABLE)) {
            // Not configurable: refuse to change flags unless identical.
            if ((existingProp.flags & (PROP_WRITABLE | PROP_ENUMERABLE | PROP_CONFIGURABLE)) !=
                (flags & (PROP_WRITABLE | PROP_ENUMERABLE | PROP_CONFIGURABLE))) {
                return false;
            }
            // Allow value change only if writable.
            if (!(existingProp.flags & PROP_WRITABLE)) {
                return false;
            }
        }
        existingProp.value = value;
        existingProp.type_id = TypeId::OBJECT;
        // Apply new flags (spec-compatible due to checks above).
        if (existingProp.flags & PROP_CONFIGURABLE) {
            existingProp.flags = flags;
        } else {
            // Non-configurable but writable -> can transition writable->false.
            existingProp.flags = flags;
        }
        return true;
    }

    Property prop;
    prop.value = value;
    prop.type_id = TypeId::OBJECT;
    prop.flags = flags;

    storage->lookup[key] = storage->entries.size();
    storage->entries.push_back({std::string(key), prop});
    return true;
}

} // namespace runtime
} // namespace nova

// Extern "C" wrapper for Object static methods (for easier linking)
extern "C" {

int64_t nova_is_error(void* value);
std::uint64_t nova_error_get_cause(void* error);
const char* nova_error_get_name(void* error);
const char* nova_error_get_message(void* error);
const char* nova_error_get_stack(void* error);

void* nova_dynamic_object_create() {
    return nova::runtime::create_object();
}

// Alias used by Proxy.cpp and other runtime modules.
void* nova_object_create_empty() {
    return nova::runtime::create_object();
}

void nova_dynamic_object_set_tagged(
    void* object, const char* key, std::uint64_t value) {
    // Proxy dispatch: if `object` is a registered Proxy, route through
    // its `set` trap instead of touching the target directly. Defined
    // in Proxy.cpp; returns nullptr for non-proxy operands.
    extern void* nova_proxy_get_info(void*);
    extern int64_t nova_proxy_dispatch_set(void*, const char*, std::uint64_t, void*);
    if (nova_proxy_get_info(object)) {
        nova_proxy_dispatch_set(object, key, value, object);
        return;
    }
    nova::runtime::object_set(
        static_cast<nova::runtime::Object*>(object), key,
        reinterpret_cast<void*>(static_cast<std::uintptr_t>(value)));
}

// Store a raw C function pointer on the Object under the given key, tagged
// as an OBJECT JSValue. Used to attach compiled object-literal methods to
// runtime Objects so Proxy traps can dispatch them as callables. Without
// this, object-literal methods would be DCE'd by LLVM because nothing
// references them directly — only the runtime property map.
void nova_dynamic_object_set_function(void* obj, const char* key, void* fnPtr) {
    if (!obj || !key || !fnPtr) return;
    nova::runtime::JSValue fnJs = nova_value_from_object(fnPtr);
    nova::runtime::object_set(
        static_cast<nova::runtime::Object*>(obj), key,
        reinterpret_cast<void*>(static_cast<std::uintptr_t>(fnJs)));
}

// Retrieve a stored function pointer (the inverse of set_function).
// Returns nullptr if the key is missing or the stored value isn't a
// OBJECT-tagged JSValue.
void* nova_dynamic_object_get_function(void* obj, const char* key) {
    if (!obj || !key) return nullptr;
    void* raw = nova::runtime::object_get(
        static_cast<nova::runtime::Object*>(obj), key);
    if (!raw) return nullptr;
    nova::runtime::JSValue v = static_cast<nova::runtime::JSValue>(
        reinterpret_cast<std::uintptr_t>(raw));
    if ((v & nova::runtime::JS_VALUE_TAG_MASK) !=
        nova::runtime::JS_VALUE_OBJECT_TAG) {
        return nullptr;
    }
    return reinterpret_cast<void*>(static_cast<std::uintptr_t>(
        v & nova::runtime::JS_VALUE_PAYLOAD_MASK));
}

// Store a function pointer AND a closure-environment pointer under the same
// key. The env is stashed under a mangled key (__env_for_<key>) so trap
// dispatch can pass it as the trailing argument when invoking the function.
// Used by object-literal methods that capture outer-scope variables.
void nova_dynamic_object_set_function_with_env(void* obj, const char* key,
                                                void* fnPtr, void* envPtr) {
    if (!obj || !key || !fnPtr) return;
    nova_dynamic_object_set_function(obj, key, fnPtr);
    if (envPtr) {
        std::string envKey = std::string("__env_for_") + key;
        nova::runtime::object_set(
            static_cast<nova::runtime::Object*>(obj), envKey.c_str(), envPtr);
    }
}

// Retrieve the closure-environment pointer associated with a stored function.
// Returns nullptr if no env was stored for this key.
void* nova_dynamic_object_get_function_env(void* obj, const char* key) {
    if (!obj || !key) return nullptr;
    std::string envKey = std::string("__env_for_") + key;
    return nova::runtime::object_get(
        static_cast<nova::runtime::Object*>(obj), envKey.c_str());
}

// Invoke a 0-argument method stored on a runtime Object via the function-
// pointer ABI established by nova_dynamic_object_set_function[_with_env].
// Used by the HIR member-call path when the receiver is a dynamic Object
// (e.g. `revocable.revoke()`). The stored function is called as
//   int64_t (*)(void* this_arg, void* env)
// regardless of its true arity — the C ABI on Windows x64 / System V ignores
// trailing register arguments the callee does not read, so a function
// declared as `int64_t(void*)` (no env) and one declared as
// `int64_t(void*, void*)` (with env) are both callable through this signature.
// Returns the function's i64 result (NaN-boxed JSValue or raw int).
int64_t nova_dynamic_call_method_0(void* obj, const char* methodName) {
    if (!obj || !methodName) return 0;
    void* fnPtr = nova_dynamic_object_get_function(obj, methodName);
    if (!fnPtr) return 0;
    void* env = nova_dynamic_object_get_function_env(obj, methodName);
    using FnType = int64_t (*)(void*, void*);
    FnType fn = reinterpret_cast<FnType>(fnPtr);
    return fn(obj, env);
}

// Thin void*-typed wrapper used by Proxy.cpp when caching trap handlers
// (treats the value as an opaque pointer).
void nova_object_set(void* obj, const char* key, void* value) {
    nova::runtime::object_set(
        static_cast<nova::runtime::Object*>(obj), key, value);
}

void* nova_object_get(void* obj, const char* key) {
    return nova::runtime::object_get(
        static_cast<nova::runtime::Object*>(obj), key);
}

int64_t nova_object_has(void* obj, const char* key) {
    // Proxy `has` trap dispatch.
    extern void* nova_proxy_get_info(void*);
    extern int64_t nova_proxy_dispatch_has(void*, const char*);
    if (nova_proxy_get_info(obj)) {
        return nova_proxy_dispatch_has(obj, key);
    }
    return nova::runtime::object_has(
        static_cast<nova::runtime::Object*>(obj), key) ? 1 : 0;
}

int64_t nova_object_delete(void* obj, const char* key) {
    // Proxy `deleteProperty` trap dispatch.
    extern void* nova_proxy_get_info(void*);
    extern int64_t nova_proxy_dispatch_delete(void*, const char*);
    if (nova_proxy_get_info(obj)) {
        return nova_proxy_dispatch_delete(obj, key);
    }
    nova::runtime::Object* o = static_cast<nova::runtime::Object*>(obj);
    if (!o || !key) return 1; // missing → true (delete is idempotent)
    // Spec: deleting a non-existent property succeeds.
    if (!nova::runtime::object_has(o, key)) return 1;
    // Capture storage size before to detect if removal happened.
    size_t before = 0;
    if (o->properties) {
        before = static_cast<nova::runtime::PropertyStorage*>(o->properties)->entries.size();
    }
    nova::runtime::object_delete(o, key);
    size_t after = o->properties
        ? static_cast<nova::runtime::PropertyStorage*>(o->properties)->entries.size()
        : 0;
    return (after < before) ? 1 : 0;
}

std::uint64_t nova_dynamic_object_get_tagged(
    void* object, const char* key) {
    if (!object || !key) {
        return nova::runtime::JS_VALUE_UNDEFINED;
    }
    // Proxy `get` trap dispatch — overrides the default target read.
    extern void* nova_proxy_get_info(void*);
    extern std::uint64_t nova_proxy_dispatch_get(void*, const char*, void*);
    if (nova_proxy_get_info(object)) {
        return nova_proxy_dispatch_get(object, key, object);
    }
    if (nova_is_error(object)) {
        if (std::strcmp(key, "cause") == 0) {
            return nova_error_get_cause(object);
        }
        if (std::strcmp(key, "name") == 0) {
            return nova_value_from_string(nova_error_get_name(object));
        }
        if (std::strcmp(key, "message") == 0) {
            return nova_value_from_string(nova_error_get_message(object));
        }
        if (std::strcmp(key, "stack") == 0) {
            return nova_value_from_string(nova_error_get_stack(object));
        }
        return nova::runtime::JS_VALUE_UNDEFINED;
    }
    auto* dynamicObject = static_cast<nova::runtime::Object*>(object);
    if (!nova::runtime::object_has(dynamicObject, key)) {
        return nova::runtime::JS_VALUE_UNDEFINED;
    }
    // All values stored via nova_dynamic_object_set_tagged are NaN-boxed
    // JSValues (numbers stored as IEEE 754 double bits with no special tag,
    // strings/objects/bools with the appropriate tag). Return the raw bits
    // unchanged so numbers don't get mis-wrapped as Object pointers.
    std::uint64_t raw = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(
        nova::runtime::object_get(dynamicObject, key)));
    return raw;
}

// Object.values(obj) - returns array of object's ENUMERABLE OWN property values (ES2017)
void* nova_object_values(void* obj_ptr) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);

    if (!obj || !obj->properties) {
        nova::runtime::ValueArray* emptyArray = nova::runtime::create_value_array(0);
        return nova::runtime::create_metadata_from_value_array(emptyArray);
    }

    auto* storage = static_cast<nova::runtime::PropertyStorage*>(obj->properties);

    int64_t count = 0;
    for (const auto& entry : storage->entries) {
        if (entry.prop.flags & nova::runtime::PROP_ENUMERABLE) count++;
    }
    nova::runtime::ValueArray* resultArray = nova::runtime::create_value_array(count);
    resultArray->length = count;

    int64_t index = 0;
    for (const auto& entry : storage->entries) {
        if (!(entry.prop.flags & nova::runtime::PROP_ENUMERABLE)) continue;
        // The stored value is a NaN-boxed JSValue reinterpreted as void*.
        // For numeric values, extract the raw integer so direct i64
        // comparisons in HIR (`values[0] != 10`) succeed. For tagged
        // pointers (strings/objects), keep the raw pointer bits.
        nova::runtime::JSValue v =
            static_cast<nova::runtime::JSValue>(reinterpret_cast<std::uintptr_t>(
                entry.prop.value));
        int64_t stored = 0;
        if (v == nova::runtime::JS_VALUE_UNDEFINED || v == nova::runtime::JS_VALUE_NULL) {
            stored = 0;
        } else if (nova::runtime::js_value_has_tag(v, nova::runtime::JS_VALUE_STRING_TAG) ||
                   nova::runtime::js_value_has_tag(v, nova::runtime::JS_VALUE_OBJECT_TAG)) {
            stored = static_cast<int64_t>(v & nova::runtime::JS_VALUE_PAYLOAD_MASK);
        } else {
            // Number (IEEE 754 double bits). Convert to integer if it's a
            // whole number, otherwise leave the raw bits (callers comparing
            // to integer literals will still match for whole numbers).
            double d = *reinterpret_cast<const double*>(&v);
            if (std::isnan(d) || std::isinf(d)) {
                stored = static_cast<int64_t>(v);
            } else if (d == std::floor(d) && std::fabs(d) < 9.007e15) {
                stored = static_cast<int64_t>(d);
            } else {
                stored = static_cast<int64_t>(v);
            }
        }
        resultArray->elements[index] = stored;
        index++;
    }

    return nova::runtime::create_metadata_from_value_array(resultArray);
}

// Object.keys(obj) - returns array of object's ENUMERABLE OWN property keys (ES2015)
void* nova_object_keys(void* obj_ptr) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);

    if (!obj || !obj->properties) {
        // Return empty array for null object or object with no properties
        nova::runtime::ValueArray* emptyArray = nova::runtime::create_value_array(0);
        return nova::runtime::create_metadata_from_value_array(emptyArray);
    }

    auto* storage = static_cast<nova::runtime::PropertyStorage*>(obj->properties);

    // Count enumerable own properties first.
    int64_t count = 0;
    for (const auto& entry : storage->entries) {
        if (entry.prop.flags & nova::runtime::PROP_ENUMERABLE) {
            count++;
        }
    }

    nova::runtime::ValueArray* resultArray = nova::runtime::create_value_array(count);
    resultArray->length = count;

    // Extract keys (property names) from enumerable own properties
    int64_t index = 0;
    for (const auto& entry : storage->entries) {
        if (!(entry.prop.flags & nova::runtime::PROP_ENUMERABLE)) continue;
        const std::string& key = entry.key;
        char* keyCopy = new char[key.length() + 1];
        std::strcpy(keyCopy, key.c_str());
        resultArray->elements[index] = reinterpret_cast<int64_t>(keyCopy);
        index++;
    }

    return nova::runtime::create_metadata_from_value_array(resultArray);
}

// Object.entries(obj) - returns array of [key, value] pairs of ENUMERABLE OWN properties (ES2017)
void* nova_object_entries(void* obj_ptr) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);

    if (!obj || !obj->properties) {
        nova::runtime::ValueArray* emptyArray = nova::runtime::create_value_array(0);
        return nova::runtime::create_metadata_from_value_array(emptyArray);
    }

    auto* storage = static_cast<nova::runtime::PropertyStorage*>(obj->properties);

    int64_t count = 0;
    for (const auto& entry : storage->entries) {
        if (entry.prop.flags & nova::runtime::PROP_ENUMERABLE) count++;
    }
    nova::runtime::ValueArray* resultArray = nova::runtime::create_value_array(count);
    resultArray->length = count;

    int64_t index = 0;
    for (const auto& entry : storage->entries) {
        if (!(entry.prop.flags & nova::runtime::PROP_ENUMERABLE)) continue;
        nova::runtime::ValueArray* entryArray = nova::runtime::create_value_array(2);
        entryArray->length = 2;

        const std::string& key = entry.key;
        char* keyCopy = new char[key.length() + 1];
        std::strcpy(keyCopy, key.c_str());
        entryArray->elements[0] = reinterpret_cast<int64_t>(keyCopy);
        // Unbox JSValue → raw i64 for numeric values (see nova_object_values).
        nova::runtime::JSValue v =
            static_cast<nova::runtime::JSValue>(reinterpret_cast<std::uintptr_t>(
                entry.prop.value));
        int64_t stored = 0;
        if (v == nova::runtime::JS_VALUE_UNDEFINED || v == nova::runtime::JS_VALUE_NULL) {
            stored = 0;
        } else if (nova::runtime::js_value_has_tag(v, nova::runtime::JS_VALUE_STRING_TAG) ||
                   nova::runtime::js_value_has_tag(v, nova::runtime::JS_VALUE_OBJECT_TAG)) {
            stored = static_cast<int64_t>(v & nova::runtime::JS_VALUE_PAYLOAD_MASK);
        } else {
            double d = *reinterpret_cast<const double*>(&v);
            if (std::isnan(d) || std::isinf(d)) {
                stored = static_cast<int64_t>(v);
            } else if (d == std::floor(d) && std::fabs(d) < 9.007e15) {
                stored = static_cast<int64_t>(d);
            } else {
                stored = static_cast<int64_t>(v);
            }
        }
        entryArray->elements[1] = stored;

        void* entryMetadata = nova::runtime::create_metadata_from_value_array(entryArray);
        resultArray->elements[index] = reinterpret_cast<int64_t>(entryMetadata);
        index++;
    }

    return nova::runtime::create_metadata_from_value_array(resultArray);
}

// Object.assign(target, source) - copies properties from source to target (ES2015)
void* nova_object_assign(void* target_ptr, void* source_ptr) {
    nova::runtime::Object* target = static_cast<nova::runtime::Object*>(target_ptr);
    nova::runtime::Object* source = static_cast<nova::runtime::Object*>(source_ptr);

    if (!target) {
        // Return null if target is null
        return nullptr;
    }

    if (!source || !source->properties) {
        // Return target unchanged if source is null or has no properties
        return target_ptr;
    }

    // Lazily allocate target storage if needed (PropertyStorage tracks insertion order).
    if (!target->properties) {
        target->properties = new nova::runtime::PropertyStorage();
    }

    auto* targetStorage = static_cast<nova::runtime::PropertyStorage*>(target->properties);
    auto* sourceStorage = static_cast<nova::runtime::PropertyStorage*>(source->properties);

    // Spec: Object.assign only copies ENUMERABLE own properties from source.
    // Existing target properties that are non-writable are silently skipped
    // (CreateDataPropertyOrThrow semantics in non-strict code).
    for (const auto& entry : sourceStorage->entries) {
        if (!(entry.prop.flags & nova::runtime::PROP_ENUMERABLE)) {
            continue;
        }
        auto existing = targetStorage->lookup.find(entry.key);
        if (existing != targetStorage->lookup.end()) {
            auto& existingProp = targetStorage->entries[existing->second].prop;
            if (!(existingProp.flags & nova::runtime::PROP_WRITABLE)) {
                continue; // non-writable target — silently skip
            }
            // [[Set]] semantics: update value, preserve target's existing flags.
            existingProp.value = entry.prop.value;
        } else {
            targetStorage->lookup[entry.key] = targetStorage->entries.size();
            targetStorage->entries.push_back(entry);
        }
    }

    return target_ptr;
}

// Object.hasOwn(obj, key) - checks if object has own property (ES2022)
int64_t nova_object_hasOwn(void* obj_ptr, const char* key) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);

    if (!obj || !key) {
        // Return false (0) if object or key is null
        return 0;
    }

    if (!obj->properties) {
        // Return false (0) if object has no properties
        return 0;
    }

    auto* storage = static_cast<nova::runtime::PropertyStorage*>(obj->properties);

    bool hasProperty = storage->lookup.find(key) != storage->lookup.end();
    return hasProperty ? 1 : 0;
}

// Object.freeze(obj) - makes object immutable (ES5)
void* nova_object_freeze(void* obj_ptr) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);

    if (!obj) {
        return nullptr;
    }

    // Mark as frozen and clear all writable/configurable bits on existing props.
    obj->integrity = 2; // frozen
    if (obj->properties) {
        auto* storage = static_cast<nova::runtime::PropertyStorage*>(obj->properties);
        for (auto& entry : storage->entries) {
            entry.prop.flags &= ~(nova::runtime::PROP_WRITABLE | nova::runtime::PROP_CONFIGURABLE);
        }
    }
    return obj_ptr;
}

// Object.isFrozen(obj) - checks if object is frozen (ES5)
int64_t nova_object_isFrozen(void* obj_ptr) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);

    if (!obj) {
        // null is considered frozen
        return 1;
    }

    if (obj->integrity == 2) return 1;

    // An empty non-extensible object is considered frozen by spec.
    if (obj->integrity == 3 && (!obj->properties ||
        static_cast<nova::runtime::PropertyStorage*>(obj->properties)->entries.empty())) {
        return 1;
    }

    return 0;
}

// Object.seal(obj) - seals object, prevents add/delete properties (ES5)
void* nova_object_seal(void* obj_ptr) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);

    if (!obj) {
        return nullptr;
    }

    obj->integrity = 1; // sealed
    if (obj->properties) {
        auto* storage = static_cast<nova::runtime::PropertyStorage*>(obj->properties);
        for (auto& entry : storage->entries) {
            entry.prop.flags &= ~nova::runtime::PROP_CONFIGURABLE;
        }
    }
    return obj_ptr;
}

// Object.isSealed(obj) - checks if object is sealed (ES5)
int64_t nova_object_isSealed(void* obj_ptr) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);

    if (!obj) {
        // null is considered sealed
        return 1;
    }

    if (obj->integrity == 1 || obj->integrity == 2) return 1;

    // An empty non-extensible object is sealed by spec.
    if (obj->integrity == 3 && (!obj->properties ||
        static_cast<nova::runtime::PropertyStorage*>(obj->properties)->entries.empty())) {
        return 1;
    }

    return 0;
}

// Object.is(value1, value2) - determines if two values are the same (ES2015)
// Returns 1 (true) if values are the same, 0 (false) otherwise
// Note: For number values, this is equivalent to strict equality (===)
// Full JavaScript implementation would also handle NaN === NaN (true) and +0 !== -0
int64_t nova_object_is(int64_t value1, int64_t value2) {
    // Simple equality comparison for numeric values
    // In JavaScript, Object.is differs from === in two cases:
    // 1. Object.is(NaN, NaN) returns true (=== returns false)
    // 2. Object.is(+0, -0) returns false (=== returns true)
    // For integer values, simple equality works correctly
    return value1 == value2 ? 1 : 0;
}

// Object.is for JavaScript Number values (SameValue semantics).
int64_t nova_object_is_number(double value1, double value2) {
    if (std::isnan(value1) && std::isnan(value2)) {
        return 1;
    }
    if (value1 == 0.0 && value2 == 0.0) {
        return std::signbit(value1) == std::signbit(value2) ? 1 : 0;
    }
    return value1 == value2 ? 1 : 0;
}

// Object.is for pointer-backed objects, arrays, and functions compares identity.
int64_t nova_object_is_identity(void* value1, void* value2) {
    return value1 == value2 ? 1 : 0;
}

// Object.create(proto) - creates a new object with the specified prototype (ES5)
void* nova_object_create(void* proto_ptr) {
    // Create a new object
    nova::runtime::Object* obj = nova::runtime::create_object();
    // Wire up the [[Prototype]] link.
    obj->proto = proto_ptr;
    return obj;
}

// Object.fromEntries(iterable) - creates object from key-value pairs (ES2019)
void* nova_object_fromEntries(void* iterable_ptr) {
    // Create a new object
    nova::runtime::Object* obj = nova::runtime::create_object();

    if (!iterable_ptr) {
        return obj;
    }

    // Treat iterable_ptr as a ValueArray metadata struct.
    char* metaBytes = static_cast<char*>(iterable_ptr);
    int64_t length = *reinterpret_cast<int64_t*>(metaBytes + 24);
    int64_t* elements = *reinterpret_cast<int64_t**>(metaBytes + 40);
    if (length <= 0 || !elements) return obj;

    nova::runtime::PropertyStorage* storage = nova::runtime::get_storage(obj);
    for (int64_t i = 0; i < length; ++i) {
        // Each element is a pointer to a nested ValueArray metadata [key, value].
        void* entryMetaPtr = reinterpret_cast<void*>(static_cast<uintptr_t>(elements[i]));
        if (!entryMetaPtr) continue;
        char* entryBytes = static_cast<char*>(entryMetaPtr);
        int64_t entryLen = *reinterpret_cast<int64_t*>(entryBytes + 24);
        int64_t* entryElements = *reinterpret_cast<int64_t**>(entryBytes + 40);
        if (entryLen < 2 || !entryElements) continue;

        // ValueArray metadata has a value_encoding byte at offset 13:
        //   0 = raw pointers (each element is a char*/void*)
        //   1 = NaN-boxed JSValues (numbers as IEEE 754, strings as STRING_TAG|ptr,
        //                            objects as OBJECT_TAG|ptr, etc.)
        uint8_t valueEncoding = *reinterpret_cast<uint8_t*>(entryBytes + 13);

        const char* key = nullptr;
        nova::runtime::JSValue v = nova::runtime::JS_VALUE_UNDEFINED;

        if (valueEncoding == 1) {
            // JSValue array. Key is a STRING_TAG|ptr boxed JSValue.
            std::uint64_t keyValue = static_cast<std::uint64_t>(entryElements[0]);
            if ((keyValue & nova::runtime::JS_VALUE_TAG_MASK) ==
                nova::runtime::JS_VALUE_STRING_TAG) {
                key = reinterpret_cast<const char*>(static_cast<std::uintptr_t>(
                    keyValue & nova::runtime::JS_VALUE_PAYLOAD_MASK));
            } else {
                // Fallback: treat as raw pointer.
                key = reinterpret_cast<const char*>(
                    static_cast<std::uintptr_t>(keyValue));
            }
            // Value is already a fully-formed NaN-boxed JSValue — keep it as-is.
            v = static_cast<nova::runtime::JSValue>(entryElements[1]);
        } else {
            // Raw pointer array. Key and value are both char*/void*.
            key = reinterpret_cast<const char*>(
                static_cast<std::uintptr_t>(entryElements[0]));
            void* valuePtr = reinterpret_cast<void*>(
                static_cast<std::uintptr_t>(entryElements[1]));
            // Best-effort: assume string. (Mixed raw-pointer arrays of objects
            // are not currently produced by HIR for fromEntries literals.)
            v = nova_value_from_string(static_cast<const char*>(valuePtr));
        }

        if (!key) continue;
        nova::runtime::object_set(
            obj, key,
            reinterpret_cast<void*>(static_cast<std::uintptr_t>(v)));
        (void)storage;
    }

    return obj;
}

// Object.getOwnPropertyNames(obj) - returns array of all property names (ES5)
// Returns own (not inherited) property names in insertion order.
void* nova_object_getOwnPropertyNames(void* obj_ptr) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);

    if (!obj || !obj->properties) {
        nova::runtime::ValueArray* emptyArray = nova::runtime::create_value_array(0);
        return nova::runtime::create_metadata_from_value_array(emptyArray);
    }

    auto* storage = static_cast<nova::runtime::PropertyStorage*>(obj->properties);
    int64_t count = static_cast<int64_t>(storage->entries.size());
    nova::runtime::ValueArray* resultArray = nova::runtime::create_value_array(count);
    resultArray->length = count;

    int64_t index = 0;
    for (const auto& entry : storage->entries) {
        const std::string& key = entry.key;
        char* keyCopy = new char[key.length() + 1];
        std::strcpy(keyCopy, key.c_str());
        resultArray->elements[index] = reinterpret_cast<int64_t>(keyCopy);
        index++;
    }

    return nova::runtime::create_metadata_from_value_array(resultArray);
}

// Object.getOwnPropertySymbols(obj) - returns array of symbol properties (ES2015)
void* nova_object_getOwnPropertySymbols(void* obj_ptr) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);
    if (!obj || !obj->properties) {
        nova::runtime::ValueArray* emptyArray = nova::runtime::create_value_array(0);
        return nova::runtime::create_metadata_from_value_array(emptyArray);
    }
    auto* storage = static_cast<nova::runtime::PropertyStorage*>(obj->properties);
    int64_t count = static_cast<int64_t>(storage->symbolEntries.size());
    nova::runtime::ValueArray* resultArray = nova::runtime::create_value_array(count);
    resultArray->length = count;
    for (int64_t i = 0; i < count; ++i) {
        resultArray->elements[i] = reinterpret_cast<int64_t>(
            storage->symbolEntries[i].symbol);
    }
    return nova::runtime::create_metadata_from_value_array(resultArray);
}

// Set a symbol-keyed property. Used by HIR when `object[symbol] = X` is
// detected — keeps symbol identity stable for `Reflect.ownKeys` and the
// `=== symbol` comparison.
void nova_object_set_symbol(void* obj_ptr, void* symbol, std::uint64_t value) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);
    if (!obj || !symbol) return;
    if (!obj->properties) {
        obj->properties = new nova::runtime::PropertyStorage();
    }
    auto* storage = static_cast<nova::runtime::PropertyStorage*>(obj->properties);
    auto it = storage->symbolLookup.find(symbol);
    nova::runtime::Property prop;
    prop.value = reinterpret_cast<void*>(static_cast<std::uintptr_t>(value));
    prop.type_id = nova::runtime::TypeId::OBJECT;
    prop.flags = nova::runtime::PROP_DEFAULT_FLAGS;
    if (it != storage->symbolLookup.end()) {
        prop.flags = storage->symbolEntries[it->second].prop.flags;
        storage->symbolEntries[it->second].prop = prop;
    } else {
        storage->symbolLookup[symbol] = storage->symbolEntries.size();
        storage->symbolEntries.push_back({symbol, prop});
    }
}

// Read a symbol-keyed property as a JSValue-tagged bits.
std::uint64_t nova_object_get_symbol(void* obj_ptr, void* symbol) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);
    if (!obj || !symbol || !obj->properties) {
        return nova::runtime::JS_VALUE_UNDEFINED;
    }
    auto* storage = static_cast<nova::runtime::PropertyStorage*>(obj->properties);
    auto it = storage->symbolLookup.find(symbol);
    if (it == storage->symbolLookup.end()) {
        return nova::runtime::JS_VALUE_UNDEFINED;
    }
    return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(
        storage->symbolEntries[it->second].prop.value));
}

int64_t nova_object_has_symbol(void* obj_ptr, void* symbol) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);
    if (!obj || !symbol || !obj->properties) return 0;
    auto* storage = static_cast<nova::runtime::PropertyStorage*>(obj->properties);
    return storage->symbolLookup.find(symbol) != storage->symbolLookup.end() ? 1 : 0;
}

// Object.getPrototypeOf(obj) - returns the prototype of an object (ES5)
void* nova_object_getPrototypeOf(void* obj_ptr) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);
    if (!obj) return nullptr;
    return obj->proto;
}

// Object.setPrototypeOf(obj, proto) - sets the prototype of an object (ES2015)
void* nova_object_setPrototypeOf(void* obj_ptr, void* proto_ptr) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);
    if (!obj) return obj_ptr;
    // Per spec, setPrototypeOf on a non-extensible object is a no-op (returns the object).
    if (obj->integrity != 0) {
        return obj_ptr;
    }
    obj->proto = proto_ptr;
    return obj_ptr;
}

// Object.isExtensible(obj) - checks if object is extensible (ES5)
int64_t nova_object_isExtensible(void* obj_ptr) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);

    if (!obj) {
        // null/undefined are not extensible
        return 0;
    }

    // integrity != 0 means non-extensible (sealed=1, frozen=2, non-extensible=3)
    return obj->integrity == 0 ? 1 : 0;
}

// Object.preventExtensions(obj) - prevents new properties from being added (ES5)
void* nova_object_preventExtensions(void* obj_ptr) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);
    if (!obj) return nullptr;
    if (obj->integrity == 0) {
        obj->integrity = 3; // non-extensible
    }
    return obj_ptr;
}

// Object.defineProperty(obj, prop, descriptor) - defines a property (ES5)
void* nova_object_defineProperty(void* obj_ptr, const char* prop, void* descriptor_ptr) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);

    if (!obj || !prop) {
        return obj_ptr;
    }

    if (!descriptor_ptr) {
        return obj_ptr;
    }

    // Read fields from the descriptor object.
    nova::runtime::Object* descriptor = static_cast<nova::runtime::Object*>(descriptor_ptr);

    // Helper: read a field as a JSValue and convert to bool via the runtime
    // ToBoolean. Descriptor values are stored as JSValues (NaN-boxed), so a
    // bare void* truthiness check is wrong — JS_VALUE_FALSE is non-null.
    auto field_as_bool = [&](const char* key) -> bool {
        if (!nova::runtime::object_has(descriptor, key)) return false;
        void* raw = nova::runtime::object_get(descriptor, key);
        // JSValues flow through property storage as the low 48 bits of a pointer.
        nova::runtime::JSValue v =
            static_cast<nova::runtime::JSValue>(reinterpret_cast<std::uintptr_t>(raw));
        // JS_VALUE_TRUE tag is 0x7ffc; everything else with the tag pattern is
        // truthy iff it isn't JS_VALUE_FALSE/UNDEFINED/NULL.
        return nova::runtime::js_value_has_tag(v, nova::runtime::JS_VALUE_TRUE) ||
               (!nova::runtime::js_value_has_tag(v, nova::runtime::JS_VALUE_FALSE) &&
                !nova::runtime::js_value_has_tag(v, nova::runtime::JS_VALUE_UNDEFINED) &&
                !nova::runtime::js_value_has_tag(v, nova::runtime::JS_VALUE_NULL) &&
                !nova::runtime::js_value_has_tag(v, nova::runtime::JS_VALUE_CANONICAL_NAN));
    };
    auto has_field = [&](const char* key) -> bool {
        return nova::runtime::object_has(descriptor, key);
    };

    // Spec semantics: when a descriptor omits an attribute, the existing
    // (or default) value is preserved. We patch the existing Property in
    // place when present, otherwise start from spec defaults for a new
    // property (writable=false, enumerable=false, configurable=false).
    nova::runtime::PropertyStorage* storage = obj->properties
        ? static_cast<nova::runtime::PropertyStorage*>(obj->properties)
        : nullptr;
    nova::runtime::Property* existingProp = nullptr;
    if (storage) {
        auto it = storage->lookup.find(prop);
        if (it != storage->lookup.end()) {
            existingProp = &storage->entries[it->second].prop;
        }
    }
    if (!storage) {
        storage = nova::runtime::get_storage(obj);
    }

    uint32_t flags = existingProp
        ? existingProp->flags
        : uint32_t(0);
    void* value = existingProp ? existingProp->value : nullptr;

    if (has_field("writable")) {
        if (field_as_bool("writable")) flags |= nova::runtime::PROP_WRITABLE;
        else flags &= ~nova::runtime::PROP_WRITABLE;
    }
    if (has_field("enumerable")) {
        if (field_as_bool("enumerable")) flags |= nova::runtime::PROP_ENUMERABLE;
        else flags &= ~nova::runtime::PROP_ENUMERABLE;
    }
    if (has_field("configurable")) {
        if (field_as_bool("configurable")) flags |= nova::runtime::PROP_CONFIGURABLE;
        else flags &= ~nova::runtime::PROP_CONFIGURABLE;
    }
    if (has_field("value")) {
        value = nova::runtime::object_get(descriptor, "value");
    }

    nova::runtime::object_define_own(obj, prop, value, flags);

    return obj_ptr;
}

// Object.defineProperties(obj, props) - defines multiple properties (ES5)
void* nova_object_defineProperties(void* obj_ptr, void* props_ptr) {
    if (!obj_ptr || !props_ptr) return obj_ptr;
    // Iterate keys of props; for each key, call defineProperty.
    nova::runtime::Object* props = static_cast<nova::runtime::Object*>(props_ptr);
    if (!props->properties) return obj_ptr;
    auto* propsStorage = static_cast<nova::runtime::PropertyStorage*>(props->properties);
    for (const auto& entry : propsStorage->entries) {
        nova_object_defineProperty(obj_ptr, entry.key.c_str(), entry.prop.value);
    }
    return obj_ptr;
}

// Object.getOwnPropertyDescriptor(obj, prop) - gets property descriptor (ES5)
void* nova_object_getOwnPropertyDescriptor(void* obj_ptr, const char* prop) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);

    if (!obj || !prop || !obj->properties) {
        return nullptr;
    }

    auto* storage = static_cast<nova::runtime::PropertyStorage*>(obj->properties);
    auto it = storage->lookup.find(prop);

    if (it == storage->lookup.end()) {
        return nullptr;
    }

    nova::runtime::Property& p = storage->entries[it->second].prop;

    // Build a descriptor object with value/writable/enumerable/configurable.
    // Boolean fields are stored as NaN-boxed JSValue (JS_VALUE_TRUE / FALSE)
    // so the HIR-generated `descriptor.writable === true` etc. matches.
    nova::runtime::Object* descriptor = nova::runtime::create_object();
    nova::runtime::object_set(descriptor, "value", p.value);
    auto box_bool = [](bool b) -> void* {
        nova::runtime::JSValue v = b
            ? nova::runtime::JS_VALUE_TRUE
            : nova::runtime::JS_VALUE_FALSE;
        return reinterpret_cast<void*>(static_cast<std::uintptr_t>(v));
    };
    nova::runtime::object_set(descriptor, "writable", box_bool(p.flags & nova::runtime::PROP_WRITABLE));
    nova::runtime::object_set(descriptor, "enumerable", box_bool(p.flags & nova::runtime::PROP_ENUMERABLE));
    nova::runtime::object_set(descriptor, "configurable", box_bool(p.flags & nova::runtime::PROP_CONFIGURABLE));
    return descriptor;
}

// Object.getOwnPropertyDescriptors(obj) - gets all property descriptors (ES2017)
void* nova_object_getOwnPropertyDescriptors(void* obj_ptr) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);
    nova::runtime::Object* result = nova::runtime::create_object();
    if (!obj || !obj->properties) return result;

    auto* storage = static_cast<nova::runtime::PropertyStorage*>(obj->properties);
    for (const auto& entry : storage->entries) {
        void* desc = nova_object_getOwnPropertyDescriptor(obj_ptr, entry.key.c_str());
        // Tag the descriptor Object* so reads via nova_dynamic_object_get_tagged
        // see a proper JSValue OBJECT box and downstream nova_value_to_object
        // unboxing succeeds (otherwise it returns nullptr for untagged ptrs).
        nova::runtime::JSValue descJs =
            nova_value_from_object(desc ? desc : nullptr);
        nova::runtime::object_set(
            result, entry.key.c_str(),
            reinterpret_cast<void*>(static_cast<std::uintptr_t>(descJs)));
    }
    return result;
}

// Object.groupBy(items, callbackFn) - groups items by key (ES2024)
// Callback signature: int64_t (*)(int64_t). The arrow-function callback
// expects a tagged JSValue (its parameter has no annotation, so HIR uses
// JSValue as the default). Array elements are raw i64 values, so we wrap
// them with nova_value_from_i64 before invoking the callback.
void* nova_object_groupBy(void* items_ptr, void* callback_ptr) {
    using GroupCallback = int64_t (*)(int64_t);
    GroupCallback callback = reinterpret_cast<GroupCallback>(callback_ptr);


    nova::runtime::Object* result = nova::runtime::create_object();
    if (!items_ptr || !callback) return result;

    // Treat the items pointer as a ValueArray metadata struct.
    char* metaBytes = static_cast<char*>(items_ptr);
    int64_t length = *reinterpret_cast<int64_t*>(metaBytes + 24);
    int64_t* elements = *reinterpret_cast<int64_t**>(metaBytes + 40);
    if (length <= 0 || !elements) return result;

    for (int64_t i = 0; i < length; ++i) {
        const int64_t element = elements[i];
        // Tag the element appropriately before invoking the callback.
        // String literals lower to i64(ptrtoint(ptr @.str to i64)); those
        // need JS_VALUE_STRING_TAG so the callback's .length / string
        // operations resolve. Numeric elements wrap as doubles.
        std::uint64_t elementJs = nova_value_from_i64(element);
        {
            bool tagged = false;
            if (static_cast<std::uint64_t>(element) > 0x10000) {
                const char* strPtr = reinterpret_cast<const char*>(
                    static_cast<std::uintptr_t>(element));
                std::string tmp;
                bool ok = true;
                for (int k = 0; k < 64; ++k) {
                    char c = strPtr[k];
                    if (c == 0) break;
                    if (c < 0x20 || c >= 0x7f) { ok = false; break; }
                    tmp.push_back(c);
                }
                if (ok && !tmp.empty()) {
                    elementJs = nova_value_from_string(tmp.c_str());
                    tagged = true;
                }
            }
            if (!tagged) {
                elementJs = nova_value_from_i64(element);
            }
        }
        const int64_t keyRaw = callback(static_cast<int64_t>(elementJs));
        // Convert the returned i64 to a string key. Heap pointers on Windows
        // x64 are well above 0x10000 and the string they point at will be a
        // C-string (arrow functions returning string literals produce these).
        // Numeric keys (small ints) are formatted with std::to_string.
        std::string key;
        if (keyRaw > 0x10000) {
            const char* strPtr =
                reinterpret_cast<const char*>(static_cast<uintptr_t>(keyRaw));
            // Read up to 64 printable ASCII bytes terminated by NUL.
            std::string tmp;
            bool ok = true;
            for (int k = 0; k < 64; ++k) {
                char c = strPtr[k];
                if (c == 0) break;
                if (c < 0x20 || c >= 0x7f) { ok = false; break; }
                tmp.push_back(c);
            }
            if (ok && !tmp.empty()) {
                key = tmp;
            } else {
                key = std::to_string(keyRaw);
            }
        } else {
            key = std::to_string(keyRaw);
        }

        // Look up or create the group's array. The stored value is a
        // JSValue-tagged OBJECT (so consumers reading via
        // nova_dynamic_object_get_tagged + nova_value_to_object unbox it
        // correctly); unbox here when re-reading on subsequent iterations.
        void* storedRaw = nova::runtime::object_get(result, key.c_str());
        void* groupMeta = storedRaw
            ? nova_value_to_object(static_cast<std::uint64_t>(
                  reinterpret_cast<std::uintptr_t>(storedRaw)))
            : nullptr;
        if (!groupMeta) {
            nova::runtime::ValueArray* groupArray =
                nova::runtime::create_value_array(4);
            groupArray->length = 0;
            groupMeta = nova::runtime::create_metadata_from_value_array(groupArray);
            nova::runtime::JSValue groupJs =
                nova_value_from_object(groupMeta);
            nova::runtime::object_set(
                result, key.c_str(),
                reinterpret_cast<void*>(static_cast<std::uintptr_t>(groupJs)));
        }
        // Append element.
        char* gMetaBytes = static_cast<char*>(groupMeta);
        int64_t gLen = *reinterpret_cast<int64_t*>(gMetaBytes + 24);
        int64_t gCap = *reinterpret_cast<int64_t*>(gMetaBytes + 32);
        int64_t** gElemPtrPtr = reinterpret_cast<int64_t**>(gMetaBytes + 40);
        if (gLen >= gCap) {
            nova::runtime::ValueArray* growArr =
                static_cast<nova::runtime::ValueArray*>(
                    static_cast<void*>(gMetaBytes));
            nova::runtime::resize_value_array(
                growArr, std::max<int64_t>(gCap * 2, 8));
            // re-read after resize (elements ptr may have changed)
            gCap = *reinterpret_cast<int64_t*>(gMetaBytes + 32);
            gElemPtrPtr = reinterpret_cast<int64_t**>(gMetaBytes + 40);
        }
        int64_t* gElements = *gElemPtrPtr;
        gElements[gLen] = element;
        *reinterpret_cast<int64_t*>(gMetaBytes + 24) = gLen + 1;
    }
    return result;
}

// ============== Instance Methods ==============

// Object.prototype.hasOwnProperty(prop) - checks if object has own property (ES1)
int64_t nova_object_hasOwnProperty(void* obj_ptr, const char* prop) {
    return nova_object_hasOwn(obj_ptr, prop);
}

// Object.prototype.isPrototypeOf(obj) - checks if object is prototype of another (ES1)
int64_t nova_object_isPrototypeOf(void* obj_ptr, void* other_ptr) {
    nova::runtime::Object* proto = static_cast<nova::runtime::Object*>(obj_ptr);
    nova::runtime::Object* descendant = static_cast<nova::runtime::Object*>(other_ptr);
    if (!proto || !descendant) return 0;

    // Walk descendant's proto chain; if we find proto, return true.
    nova::runtime::Object* current = static_cast<nova::runtime::Object*>(descendant->proto);
    int depth = 0;
    while (current) {
        if (current == proto) return 1;
        current = static_cast<nova::runtime::Object*>(current->proto);
        if (++depth > 4096) break; // cycle guard
    }
    return 0;
}

// Object.prototype.propertyIsEnumerable(prop) - checks if property is enumerable (ES1)
int64_t nova_object_propertyIsEnumerable(void* obj_ptr, const char* prop) {
    // All own properties are enumerable in our simplified model
    return nova_object_hasOwn(obj_ptr, prop);
}

// Object.prototype.toString() - returns string representation (ES1)
const char* nova_object_toString(void* obj_ptr) {
    if (!obj_ptr) {
        char* result = new char[16];
        std::strcpy(result, "[object Null]");
        return result;
    }

    char* result = new char[16];
    std::strcpy(result, "[object Object]");
    return result;
}

// Object.prototype.toLocaleString() - returns locale string representation (ES1)
const char* nova_object_toLocaleString(void* obj_ptr) {
    // Same as toString for objects
    return nova_object_toString(obj_ptr);
}

// Object.prototype.valueOf() - returns primitive value (ES1)
void* nova_object_valueOf(void* obj_ptr) {
    // Return the object itself
    return obj_ptr;
}

// Create a keys array from compile-time known string constants
// Used by for-in loops when iterating over struct-based object literals
// count: number of keys
// keys: array of C-string pointers (const char**)
void* nova_create_keys_array(int64_t count, const char** keys) {
    nova::runtime::ValueArray* resultArray = nova::runtime::create_value_array(count);
    resultArray->length = count;

    for (int64_t i = 0; i < count; i++) {
        // Store the key string pointer as i64
        resultArray->elements[i] = reinterpret_cast<int64_t>(keys[i]);
    }

    return nova::runtime::create_metadata_from_value_array(resultArray);
}

// ============================================================================
// Class static property storage (mutable runtime backing).
// Static class properties need a runtime home because their values can change
// at runtime (e.g. a private static counter incremented in the constructor).
// Keyed by (className, fieldName) - both strings.
// ============================================================================
} // extern "C"

#include <mutex>
#include <unordered_map>

static std::unordered_map<std::string, int64_t>& classStaticStore() {
    static std::unordered_map<std::string, int64_t> store;
    return store;
}

static std::string classStaticKey(const char* className, const char* fieldName) {
    return std::string(className ? className : "") + "::" +
           std::string(fieldName ? fieldName : "");
}

extern "C" {

// Initialize a static property if it hasn't been initialized yet.
// Idempotent - safe to call on every class declaration encounter.
void nova_class_static_init_i64(const char* className, const char* fieldName, int64_t value) {
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    auto& store = classStaticStore();
    std::string key = classStaticKey(className, fieldName);
    if (store.find(key) == store.end()) {
        store[key] = value;
    }
}

int64_t nova_class_static_get_i64(const char* className, const char* fieldName) {
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    auto& store = classStaticStore();
    std::string key = classStaticKey(className, fieldName);
    auto it = store.find(key);
    if (it == store.end()) return 0;
    return it->second;
}

// Lazy init variant: returns the current value, initializing to defaultValue if not yet set.
int64_t nova_class_static_get_or_init_i64(const char* className, const char* fieldName, int64_t defaultValue) {
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    auto& store = classStaticStore();
    std::string key = classStaticKey(className, fieldName);
    auto it = store.find(key);
    if (it == store.end()) {
        store[key] = defaultValue;
        return defaultValue;
    }
    return it->second;
}

void nova_class_static_set_i64(const char* className, const char* fieldName, int64_t value) {
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    classStaticStore()[classStaticKey(className, fieldName)] = value;
}

} // extern "C"
