// Map Runtime Implementation for Nova Compiler
// ES2015 (ES6) Map collection

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <vector>
#include <string>

// Forward declarations for Nova runtime functions
extern "C" {
    void* nova_create_array(int64_t size);
    void nova_array_push(void* arr, int64_t value);
    void nova_array_push_string(void* arr, const char* value);
}

// Map entry structure - stores key-value pairs
struct NovaMapEntry {
    enum class KeyType { Number, String, Boolean, Null, Undefined, Object };
    KeyType keyType;
    union {
        int64_t numKey;
        char* strKey;
    };
    enum class ValueType { Number, String, Boolean, Null, Undefined, Object };
    ValueType valueType;
    union {
        int64_t numValue;
        char* strValue;
    };
    bool deleted;  // For maintaining iteration order during deletions
};

// Nova Map structure
struct NovaMap {
    std::vector<NovaMapEntry>* entries;
    int64_t size;  // Active (non-deleted) entries count
};

// Helper: Compare keys
static bool keysEqual(const NovaMapEntry& entry, NovaMapEntry::KeyType keyType, int64_t numKey, const char* strKey) {
    if (entry.deleted) return false;
    if (entry.keyType != keyType) return false;

    switch (keyType) {
        case NovaMapEntry::KeyType::Number:
        case NovaMapEntry::KeyType::Boolean:
            return entry.numKey == numKey;
        case NovaMapEntry::KeyType::String:
            return strKey && entry.strKey && strcmp(entry.strKey, strKey) == 0;
        case NovaMapEntry::KeyType::Null:
        case NovaMapEntry::KeyType::Undefined:
            return true;
        default:
            return entry.numKey == numKey;  // Object reference comparison
    }
}

// Find entry index by key, returns -1 if not found
static int64_t findEntry(NovaMap* map, NovaMapEntry::KeyType keyType, int64_t numKey, const char* strKey) {
    if (!map || !map->entries) return -1;

    for (size_t i = 0; i < map->entries->size(); i++) {
        if (keysEqual((*map->entries)[i], keyType, numKey, strKey)) {
            return (int64_t)i;
        }
    }
    return -1;
}

extern "C" {

// =========================================
// Constructor: new Map()
// =========================================
void* nova_map_create() {
    NovaMap* map = new NovaMap();
    map->entries = new std::vector<NovaMapEntry>();
    map->size = 0;
    return map;
}

// =========================================
// Map.prototype.size (getter)
// =========================================
int64_t nova_map_size(void* mapPtr) {
    if (!mapPtr) return 0;
    NovaMap* map = static_cast<NovaMap*>(mapPtr);
    return map->size;
}

// =========================================
// Map.prototype.set(key, value) - Number key, Number value
// =========================================
void* nova_map_set_num_num(void* mapPtr, int64_t key, int64_t value) {
    if (!mapPtr) return mapPtr;
    NovaMap* map = static_cast<NovaMap*>(mapPtr);

    // Check if key exists
    int64_t idx = findEntry(map, NovaMapEntry::KeyType::Number, key, nullptr);
    if (idx >= 0) {
        // Update existing entry
        (*map->entries)[idx].valueType = NovaMapEntry::ValueType::Number;
        (*map->entries)[idx].numValue = value;
    } else {
        // Add new entry
        NovaMapEntry entry;
        entry.keyType = NovaMapEntry::KeyType::Number;
        entry.numKey = key;
        entry.valueType = NovaMapEntry::ValueType::Number;
        entry.numValue = value;
        entry.deleted = false;
        map->entries->push_back(entry);
        map->size++;
    }
    return mapPtr;  // Return map for chaining
}

// =========================================
// Map.prototype.set(key, value) - String key, Number value
// =========================================
void* nova_map_set_str_num(void* mapPtr, const char* key, int64_t value) {
    if (!mapPtr) return mapPtr;
    NovaMap* map = static_cast<NovaMap*>(mapPtr);

    int64_t idx = findEntry(map, NovaMapEntry::KeyType::String, 0, key);
    if (idx >= 0) {
        (*map->entries)[idx].valueType = NovaMapEntry::ValueType::Number;
        (*map->entries)[idx].numValue = value;
    } else {
        NovaMapEntry entry;
        entry.keyType = NovaMapEntry::KeyType::String;
        entry.strKey = strdup(key);
        entry.valueType = NovaMapEntry::ValueType::Number;
        entry.numValue = value;
        entry.deleted = false;
        map->entries->push_back(entry);
        map->size++;
    }
    return mapPtr;
}

// =========================================
// Map.prototype.set(key, value) - Number key, String value
// =========================================
void* nova_map_set_num_str(void* mapPtr, int64_t key, const char* value) {
    if (!mapPtr) return mapPtr;
    NovaMap* map = static_cast<NovaMap*>(mapPtr);

    int64_t idx = findEntry(map, NovaMapEntry::KeyType::Number, key, nullptr);
    if (idx >= 0) {
        if ((*map->entries)[idx].valueType == NovaMapEntry::ValueType::String) {
            free((*map->entries)[idx].strValue);
        }
        (*map->entries)[idx].valueType = NovaMapEntry::ValueType::String;
        (*map->entries)[idx].strValue = strdup(value);
    } else {
        NovaMapEntry entry;
        entry.keyType = NovaMapEntry::KeyType::Number;
        entry.numKey = key;
        entry.valueType = NovaMapEntry::ValueType::String;
        entry.strValue = strdup(value);
        entry.deleted = false;
        map->entries->push_back(entry);
        map->size++;
    }
    return mapPtr;
}

// =========================================
// Map.prototype.set(key, value) - String key, String value
// =========================================
void* nova_map_set_str_str(void* mapPtr, const char* key, const char* value) {
    if (!mapPtr) return mapPtr;
    NovaMap* map = static_cast<NovaMap*>(mapPtr);

    int64_t idx = findEntry(map, NovaMapEntry::KeyType::String, 0, key);
    if (idx >= 0) {
        if ((*map->entries)[idx].valueType == NovaMapEntry::ValueType::String) {
            free((*map->entries)[idx].strValue);
        }
        (*map->entries)[idx].valueType = NovaMapEntry::ValueType::String;
        (*map->entries)[idx].strValue = strdup(value);
    } else {
        NovaMapEntry entry;
        entry.keyType = NovaMapEntry::KeyType::String;
        entry.strKey = strdup(key);
        entry.valueType = NovaMapEntry::ValueType::String;
        entry.strValue = strdup(value);
        entry.deleted = false;
        map->entries->push_back(entry);
        map->size++;
    }
    return mapPtr;
}

// =========================================
// Map.prototype.get(key) - Number key, returns Number
// =========================================
int64_t nova_map_get_num(void* mapPtr, int64_t key) {
    if (!mapPtr) return 0;
    NovaMap* map = static_cast<NovaMap*>(mapPtr);

    int64_t idx = findEntry(map, NovaMapEntry::KeyType::Number, key, nullptr);
    if (idx >= 0 && (*map->entries)[idx].valueType == NovaMapEntry::ValueType::Number) {
        return (*map->entries)[idx].numValue;
    }
    return 0;  // undefined -> 0 for numbers
}

// =========================================
// Map.prototype.get(key) - String key, returns Number
// =========================================
int64_t nova_map_get_str_num(void* mapPtr, const char* key) {
    if (!mapPtr) return 0;
    NovaMap* map = static_cast<NovaMap*>(mapPtr);

    int64_t idx = findEntry(map, NovaMapEntry::KeyType::String, 0, key);
    if (idx >= 0 && (*map->entries)[idx].valueType == NovaMapEntry::ValueType::Number) {
        return (*map->entries)[idx].numValue;
    }
    return 0;
}

// =========================================
// Map.prototype.get(key) - Number key, returns String
// =========================================
char* nova_map_get_num_str(void* mapPtr, int64_t key) {
    if (!mapPtr) return strdup("undefined");
    NovaMap* map = static_cast<NovaMap*>(mapPtr);

    int64_t idx = findEntry(map, NovaMapEntry::KeyType::Number, key, nullptr);
    if (idx >= 0 && (*map->entries)[idx].valueType == NovaMapEntry::ValueType::String) {
        return strdup((*map->entries)[idx].strValue);
    }
    return strdup("undefined");
}

// =========================================
// Map.prototype.get(key) - String key, returns String
// =========================================
char* nova_map_get_str_str(void* mapPtr, const char* key) {
    if (!mapPtr) return strdup("undefined");
    NovaMap* map = static_cast<NovaMap*>(mapPtr);

    int64_t idx = findEntry(map, NovaMapEntry::KeyType::String, 0, key);
    if (idx >= 0 && (*map->entries)[idx].valueType == NovaMapEntry::ValueType::String) {
        return strdup((*map->entries)[idx].strValue);
    }
    return strdup("undefined");
}

// =========================================
// Map.prototype.has(key) - Number key
// =========================================
int64_t nova_map_has_num(void* mapPtr, int64_t key) {
    if (!mapPtr) return 0;
    NovaMap* map = static_cast<NovaMap*>(mapPtr);
    return findEntry(map, NovaMapEntry::KeyType::Number, key, nullptr) >= 0 ? 1 : 0;
}

// =========================================
// Map.prototype.has(key) - String key
// =========================================
int64_t nova_map_has_str(void* mapPtr, const char* key) {
    if (!mapPtr) return 0;
    NovaMap* map = static_cast<NovaMap*>(mapPtr);
    return findEntry(map, NovaMapEntry::KeyType::String, 0, key) >= 0 ? 1 : 0;
}

// =========================================
// Map.prototype.delete(key) - Number key
// =========================================
int64_t nova_map_delete_num(void* mapPtr, int64_t key) {
    if (!mapPtr) return 0;
    NovaMap* map = static_cast<NovaMap*>(mapPtr);

    int64_t idx = findEntry(map, NovaMapEntry::KeyType::Number, key, nullptr);
    if (idx >= 0) {
        NovaMapEntry& entry = (*map->entries)[idx];
        if (entry.valueType == NovaMapEntry::ValueType::String) {
            free(entry.strValue);
        }
        entry.deleted = true;
        map->size--;
        return 1;  // true
    }
    return 0;  // false
}

// =========================================
// Map.prototype.delete(key) - String key
// =========================================
int64_t nova_map_delete_str(void* mapPtr, const char* key) {
    if (!mapPtr) return 0;
    NovaMap* map = static_cast<NovaMap*>(mapPtr);

    int64_t idx = findEntry(map, NovaMapEntry::KeyType::String, 0, key);
    if (idx >= 0) {
        NovaMapEntry& entry = (*map->entries)[idx];
        if (entry.keyType == NovaMapEntry::KeyType::String) {
            free(entry.strKey);
        }
        if (entry.valueType == NovaMapEntry::ValueType::String) {
            free(entry.strValue);
        }
        entry.deleted = true;
        map->size--;
        return 1;
    }
    return 0;
}

// =========================================
// Map.prototype.clear()
// =========================================
void nova_map_clear(void* mapPtr) {
    if (!mapPtr) return;
    NovaMap* map = static_cast<NovaMap*>(mapPtr);

    // Free string keys and values
    for (auto& entry : *map->entries) {
        if (!entry.deleted) {
            if (entry.keyType == NovaMapEntry::KeyType::String) {
                free(entry.strKey);
            }
            if (entry.valueType == NovaMapEntry::ValueType::String) {
                free(entry.strValue);
            }
        }
    }
    map->entries->clear();
    map->size = 0;
}

// =========================================
// Map.prototype.keys() - Returns array of keys
// =========================================
void* nova_map_keys(void* mapPtr) {
    void* arr = nova_create_array(0);
    if (!mapPtr) return arr;
    NovaMap* map = static_cast<NovaMap*>(mapPtr);

    for (const auto& entry : *map->entries) {
        if (!entry.deleted) {
            if (entry.keyType == NovaMapEntry::KeyType::Number) {
                nova_array_push(arr, entry.numKey);
            } else if (entry.keyType == NovaMapEntry::KeyType::String) {
                nova_array_push_string(arr, entry.strKey);
            }
        }
    }
    return arr;
}

// =========================================
// Map.prototype.values() - Returns array of values
// =========================================
void* nova_map_values(void* mapPtr) {
    void* arr = nova_create_array(0);
    if (!mapPtr) return arr;
    NovaMap* map = static_cast<NovaMap*>(mapPtr);

    for (const auto& entry : *map->entries) {
        if (!entry.deleted) {
            if (entry.valueType == NovaMapEntry::ValueType::Number) {
                nova_array_push(arr, entry.numValue);
            } else if (entry.valueType == NovaMapEntry::ValueType::String) {
                nova_array_push_string(arr, entry.strValue);
            }
        }
    }
    return arr;
}

// =========================================
// Map.prototype.entries() - Returns array of [key, value] pairs
// =========================================
void* nova_map_entries(void* mapPtr) {
    void* arr = nova_create_array(0);
    if (!mapPtr) return arr;
    NovaMap* map = static_cast<NovaMap*>(mapPtr);

    // For simplicity, we return an array where even indices are keys, odd are values
    // A more complete implementation would return actual tuple arrays
    for (const auto& entry : *map->entries) {
        if (!entry.deleted) {
            // Push key
            if (entry.keyType == NovaMapEntry::KeyType::Number) {
                nova_array_push(arr, entry.numKey);
            } else if (entry.keyType == NovaMapEntry::KeyType::String) {
                nova_array_push_string(arr, entry.strKey);
            }
            // Push value
            if (entry.valueType == NovaMapEntry::ValueType::Number) {
                nova_array_push(arr, entry.numValue);
            } else if (entry.valueType == NovaMapEntry::ValueType::String) {
                nova_array_push_string(arr, entry.strValue);
            }
        }
    }
    return arr;
}

// =========================================
// Map.prototype.forEach(callback)
// Note: Callback support requires function pointer handling
// For now, this is a placeholder that iterates internally
// =========================================
void nova_map_foreach(void* mapPtr, void* callback) {
    if (!mapPtr) return;
    // NovaMap* map = static_cast<NovaMap*>(mapPtr);
    // Full callback implementation would require function pointer invocation
    // This is handled at the HIR level with inline code generation
}

// =========================================
// Map.groupBy (ES2024) - Static method
// Groups items by key returned from callback
// =========================================
void* nova_map_groupby(void* iterable, void* callback) {
    // Creates a new Map where keys are results of callback
    // and values are arrays of elements with that key
    void* result = nova_map_create();
    // Full implementation requires callback invocation support
    return result;
}

}  // extern "C"
