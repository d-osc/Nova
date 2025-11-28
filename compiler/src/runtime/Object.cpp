#include "nova/runtime/Runtime.h"
#include <cstring>
#include <unordered_map>
#include <vector>

namespace nova {
namespace runtime {

struct Property {
    void* value;
    TypeId type_id;
};

Object* create_object() {
    // Allocate object structure
    Object* obj = static_cast<Object*>(allocate(sizeof(Object), TypeId::OBJECT));
    
    // Initialize object
    obj->properties = nullptr; // Will be lazily allocated
    
    return obj;
}

void* object_get(Object* obj, const char* key) {
    if (!obj || !key) return nullptr;
    
    // Lazily allocate properties map
    if (!obj->properties) {
        return nullptr;
    }
    
    auto* properties = static_cast<std::unordered_map<std::string, Property>*>(obj->properties);
    auto it = properties->find(key);
    
    if (it != properties->end()) {
        return it->second.value;
    }
    
    return nullptr;
}

void object_set(Object* obj, const char* key, void* value) {
    if (!obj || !key) return;
    
    // Lazily allocate properties map
    if (!obj->properties) {
        obj->properties = new std::unordered_map<std::string, Property>();
    }
    
    auto* properties = static_cast<std::unordered_map<std::string, Property>*>(obj->properties);
    
    Property prop;
    prop.value = value;
    prop.type_id = TypeId::OBJECT; // Default type
    
    (*properties)[key] = prop;
}

bool object_has(Object* obj, const char* key) {
    if (!obj || !key) return false;
    
    if (!obj->properties) {
        return false;
    }
    
    auto* properties = static_cast<std::unordered_map<std::string, Property>*>(obj->properties);
    return properties->find(key) != properties->end();
}

void object_delete(Object* obj, const char* key) {
    if (!obj || !key) return;

    if (!obj->properties) {
        return;
    }

    auto* properties = static_cast<std::unordered_map<std::string, Property>*>(obj->properties);
    properties->erase(key);

    // Clean up if empty
    if (properties->empty()) {
        delete properties;
        obj->properties = nullptr;
    }
}

} // namespace runtime
} // namespace nova

// Extern "C" wrapper for Object static methods (for easier linking)
extern "C" {

// Object.values(obj) - returns array of object's property values (ES2017)
void* nova_object_values(void* obj_ptr) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);

    if (!obj || !obj->properties) {
        // Return empty array for null object or object with no properties
        nova::runtime::ValueArray* emptyArray = nova::runtime::create_value_array(0);
        return nova::runtime::create_metadata_from_value_array(emptyArray);
    }

    auto* properties = static_cast<std::unordered_map<std::string, nova::runtime::Property>*>(obj->properties);

    // Create array with same size as number of properties
    int64_t count = static_cast<int64_t>(properties->size());
    nova::runtime::ValueArray* resultArray = nova::runtime::create_value_array(count);
    resultArray->length = count;

    // Extract values from properties
    int64_t index = 0;
    for (const auto& pair : *properties) {
        // For now, assume values are stored as int64_t
        // In a full implementation, we'd need to handle different types
        resultArray->elements[index] = reinterpret_cast<int64_t>(pair.second.value);
        index++;
    }

    return nova::runtime::create_metadata_from_value_array(resultArray);
}

// Object.keys(obj) - returns array of object's property keys (ES2015)
void* nova_object_keys(void* obj_ptr) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);

    if (!obj || !obj->properties) {
        // Return empty array for null object or object with no properties
        nova::runtime::ValueArray* emptyArray = nova::runtime::create_value_array(0);
        return nova::runtime::create_metadata_from_value_array(emptyArray);
    }

    auto* properties = static_cast<std::unordered_map<std::string, nova::runtime::Property>*>(obj->properties);

    // Create array with same size as number of properties
    int64_t count = static_cast<int64_t>(properties->size());
    nova::runtime::ValueArray* resultArray = nova::runtime::create_value_array(count);
    resultArray->length = count;

    // Extract keys (property names) from properties
    int64_t index = 0;
    for (const auto& pair : *properties) {
        // Store the key as a pointer to a string
        // We need to allocate and copy the string
        const char* keyStr = pair.first.c_str();
        char* keyCopy = new char[pair.first.length() + 1];
        std::strcpy(keyCopy, keyStr);
        resultArray->elements[index] = reinterpret_cast<int64_t>(keyCopy);
        index++;
    }

    return nova::runtime::create_metadata_from_value_array(resultArray);
}

// Object.entries(obj) - returns array of [key, value] pairs (ES2017)
void* nova_object_entries(void* obj_ptr) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);

    if (!obj || !obj->properties) {
        // Return empty array for null object or object with no properties
        nova::runtime::ValueArray* emptyArray = nova::runtime::create_value_array(0);
        return nova::runtime::create_metadata_from_value_array(emptyArray);
    }

    auto* properties = static_cast<std::unordered_map<std::string, nova::runtime::Property>*>(obj->properties);

    // Create array with same size as number of properties
    int64_t count = static_cast<int64_t>(properties->size());
    nova::runtime::ValueArray* resultArray = nova::runtime::create_value_array(count);
    resultArray->length = count;

    // Extract [key, value] pairs from properties
    int64_t index = 0;
    for (const auto& pair : *properties) {
        // For each entry, create a sub-array with [key, value]
        nova::runtime::ValueArray* entryArray = nova::runtime::create_value_array(2);
        entryArray->length = 2;

        // Element 0: key (string pointer)
        const char* keyStr = pair.first.c_str();
        char* keyCopy = new char[pair.first.length() + 1];
        std::strcpy(keyCopy, keyStr);
        entryArray->elements[0] = reinterpret_cast<int64_t>(keyCopy);

        // Element 1: value
        entryArray->elements[1] = reinterpret_cast<int64_t>(pair.second.value);

        // Store pointer to the entry array in the result
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

    // Lazily allocate target properties map if needed
    if (!target->properties) {
        target->properties = new std::unordered_map<std::string, nova::runtime::Property>();
    }

    auto* targetProps = static_cast<std::unordered_map<std::string, nova::runtime::Property>*>(target->properties);
    auto* sourceProps = static_cast<std::unordered_map<std::string, nova::runtime::Property>*>(source->properties);

    // Copy all properties from source to target
    for (const auto& pair : *sourceProps) {
        (*targetProps)[pair.first] = pair.second;
    }

    // Return the modified target object
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

    auto* properties = static_cast<std::unordered_map<std::string, nova::runtime::Property>*>(obj->properties);

    // Check if property exists
    bool hasProperty = properties->find(key) != properties->end();

    // Return 1 for true, 0 for false
    return hasProperty ? 1 : 0;
}

// Object.freeze(obj) - makes object immutable (ES5)
void* nova_object_freeze(void* obj_ptr) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);

    if (!obj) {
        // Return null if object is null
        return nullptr;
    }

    // Note: Full freeze functionality would prevent modifications to the object
    // For now, this is a no-op that returns the object pointer
    // Full implementation would require:
    // 1. Adding a frozen flag to the Object structure
    // 2. Checking frozen state in object_set and object_delete
    // 3. Preventing property additions/deletions/modifications

    // Return the object pointer (frozen in theory)
    return obj_ptr;
}

// Object.isFrozen(obj) - checks if object is frozen (ES5)
int64_t nova_object_isFrozen(void* obj_ptr) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);

    if (!obj) {
        // Return true (1) for null - null is considered frozen
        // (consistent with JavaScript behavior)
        return 1;
    }

    // Note: Full isFrozen functionality would check a frozen flag
    // For now, always return false (0) since freeze is a no-op
    // Full implementation would require:
    // 1. Adding a frozen flag to the Object structure
    // 2. Returning the frozen flag state

    // Return false (0) - object is not frozen
    return 0;
}

// Object.seal(obj) - seals object, prevents add/delete properties (ES5)
void* nova_object_seal(void* obj_ptr) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);

    if (!obj) {
        // Return null if object is null
        return nullptr;
    }

    // Note: Full seal functionality would prevent adding/deleting properties
    // but allow modifying existing property values (unlike freeze)
    // For now, this is a no-op that returns the object pointer
    // Full implementation would require:
    // 1. Adding a sealed flag to the Object structure
    // 2. Checking sealed state in object_set (allow modification)
    // 3. Checking sealed state in object_delete (prevent deletion)
    // 4. Preventing property additions

    // Return the object pointer (sealed in theory)
    return obj_ptr;
}

// Object.isSealed(obj) - checks if object is sealed (ES5)
int64_t nova_object_isSealed(void* obj_ptr) {
    nova::runtime::Object* obj = static_cast<nova::runtime::Object*>(obj_ptr);

    if (!obj) {
        // Return true (1) for null - null is considered sealed
        // (consistent with JavaScript behavior)
        return 1;
    }

    // Note: Full isSealed functionality would check a sealed flag
    // For now, always return false (0) since seal is a no-op
    // Full implementation would require:
    // 1. Adding a sealed flag to the Object structure
    // 2. Returning the sealed flag state

    // Return false (0) - object is not sealed
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

} // extern "C"