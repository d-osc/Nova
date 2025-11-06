#include "nova/runtime/Runtime.h"
#include <cstring>
#include <unordered_map>

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