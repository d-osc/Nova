// Reflect.cpp - ES2015 Reflect API implementation for Nova
// Provides methods for interceptable JavaScript operations

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>

extern "C" {

// Forward declarations from other runtime files
void* nova_object_create_empty();
void nova_object_set(void* obj, const char* key, void* value);
void* nova_object_get(void* obj, const char* key);
int64_t nova_object_has(void* obj, const char* key);
int64_t nova_object_delete(void* obj, const char* key);
void* nova_object_keys(void* obj);
void* nova_object_getPrototypeOf(void* obj);
void* nova_object_setPrototypeOf(void* obj, void* proto);
int64_t nova_object_isExtensible(void* obj);
void* nova_object_preventExtensions(void* obj);
void* nova_object_defineProperty(void* obj, const char* prop, void* descriptor);
void* nova_object_getOwnPropertyDescriptor(void* obj, const char* prop);
void* nova_object_getOwnPropertyNames(void* obj);
void* nova_object_getOwnPropertySymbols(void* obj);
void* nova_value_array_create();
void nova_value_array_push(void* arr, void* value);
int64_t nova_value_array_length(void* arr);
void* nova_value_array_at(void* arr, int64_t index);

// ============================================================================
// Reflect.apply(target, thisArg, argumentsList)
// Calls a target function with arguments as specified
// ============================================================================
void* nova_reflect_apply([[maybe_unused]] void* target, [[maybe_unused]] void* thisArg, [[maybe_unused]] void* argumentsList) {
    // In a full implementation, this would call the target function
    // with the specified this value and arguments
    // For now, return undefined (null)
    return nullptr;
}

// ============================================================================
// Reflect.construct(target, argumentsList[, newTarget])
// Acts like the new operator, but as a function
// ============================================================================
void* nova_reflect_construct([[maybe_unused]] void* target, [[maybe_unused]] void* argumentsList, [[maybe_unused]] void* newTarget) {
    // In a full implementation, this would construct a new instance
    // For now, create an empty object
    return nova_object_create_empty();
}

// ============================================================================
// Reflect.defineProperty(target, propertyKey, attributes)
// Similar to Object.defineProperty(), returns a Boolean
// ============================================================================
int64_t nova_reflect_defineProperty(void* target, const char* propertyKey, void* attributes) {
    if (!target || !propertyKey) return 0;

    void* result = nova_object_defineProperty(target, propertyKey, attributes);
    return result != nullptr ? 1 : 0;
}

// ============================================================================
// Reflect.deleteProperty(target, propertyKey)
// The delete operator as a function. Returns Boolean
// ============================================================================
int64_t nova_reflect_deleteProperty(void* target, const char* propertyKey) {
    if (!target || !propertyKey) return 0;

    return nova_object_delete(target, propertyKey);
}

// ============================================================================
// Reflect.get(target, propertyKey[, receiver])
// Returns the value of the property
// ============================================================================
void* nova_reflect_get(void* target, const char* propertyKey, [[maybe_unused]] void* receiver) {
    if (!target || !propertyKey) return nullptr;

    // receiver is used for getter functions - for simplicity, we use target
    return nova_object_get(target, propertyKey);
}

// ============================================================================
// Reflect.getOwnPropertyDescriptor(target, propertyKey)
// Similar to Object.getOwnPropertyDescriptor()
// ============================================================================
void* nova_reflect_getOwnPropertyDescriptor(void* target, const char* propertyKey) {
    if (!target || !propertyKey) return nullptr;

    return nova_object_getOwnPropertyDescriptor(target, propertyKey);
}

// ============================================================================
// Reflect.getPrototypeOf(target)
// Same as Object.getPrototypeOf()
// ============================================================================
void* nova_reflect_getPrototypeOf(void* target) {
    if (!target) return nullptr;

    return nova_object_getPrototypeOf(target);
}

// ============================================================================
// Reflect.has(target, propertyKey)
// Returns Boolean indicating if property exists (like in operator)
// ============================================================================
int64_t nova_reflect_has(void* target, const char* propertyKey) {
    if (!target || !propertyKey) return 0;

    return nova_object_has(target, propertyKey);
}

// ============================================================================
// Reflect.isExtensible(target)
// Same as Object.isExtensible(). Returns Boolean
// ============================================================================
int64_t nova_reflect_isExtensible(void* target) {
    if (!target) return 0;

    return nova_object_isExtensible(target);
}

// ============================================================================
// Reflect.ownKeys(target)
// Returns an array of the target object's own property keys
// ============================================================================
void* nova_reflect_ownKeys(void* target) {
    if (!target) return nova_value_array_create();

    // Get both string keys and symbol keys
    void* stringKeys = nova_object_getOwnPropertyNames(target);
    void* symbolKeys = nova_object_getOwnPropertySymbols(target);

    // Combine them into a single array
    void* result = nova_value_array_create();

    // Add string keys
    if (stringKeys) {
        int64_t len = nova_value_array_length(stringKeys);
        for (int64_t i = 0; i < len; i++) {
            void* key = nova_value_array_at(stringKeys, i);
            nova_value_array_push(result, key);
        }
    }

    // Add symbol keys
    if (symbolKeys) {
        int64_t len = nova_value_array_length(symbolKeys);
        for (int64_t i = 0; i < len; i++) {
            void* key = nova_value_array_at(symbolKeys, i);
            nova_value_array_push(result, key);
        }
    }

    return result;
}

// ============================================================================
// Reflect.preventExtensions(target)
// Similar to Object.preventExtensions(). Returns Boolean
// ============================================================================
int64_t nova_reflect_preventExtensions(void* target) {
    if (!target) return 0;

    void* result = nova_object_preventExtensions(target);
    return result != nullptr ? 1 : 0;
}

// ============================================================================
// Reflect.set(target, propertyKey, value[, receiver])
// Sets the value of a property. Returns Boolean
// ============================================================================
int64_t nova_reflect_set(void* target, const char* propertyKey, void* value, [[maybe_unused]] void* receiver) {
    if (!target || !propertyKey) return 0;

    // receiver is used for setter functions - for simplicity, we use target
    nova_object_set(target, propertyKey, value);
    return 1;
}

// ============================================================================
// Reflect.setPrototypeOf(target, prototype)
// Sets the prototype. Returns Boolean
// ============================================================================
int64_t nova_reflect_setPrototypeOf(void* target, void* prototype) {
    if (!target) return 0;

    void* result = nova_object_setPrototypeOf(target, prototype);
    return result != nullptr ? 1 : 0;
}

} // extern "C"
