// Map Runtime Implementation for Nova Compiler
// ES2015 (ES6) Map collection

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <vector>
#include <string>
#include "nova/runtime/Value.h"
#include "nova/runtime/Runtime.h"

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
// Map.prototype.get(key) - returns a NaN-boxed JSValue (or JS_VALUE_UNDEFINED).
// These variants remove the HIR's compile-time guesswork about the value type
// when the static argument types are ambiguous (e.g. `numMap.get(1)` where
// the stored value happens to be a string).
// =========================================
int64_t nova_map_get_num_jsvalue(void* mapPtr, int64_t key) {
    if (!mapPtr) return static_cast<int64_t>(nova::runtime::JS_VALUE_UNDEFINED);
    NovaMap* map = static_cast<NovaMap*>(mapPtr);
    int64_t idx = findEntry(map, NovaMapEntry::KeyType::Number, key, nullptr);
    if (idx < 0) return static_cast<int64_t>(nova::runtime::JS_VALUE_UNDEFINED);
    const auto& entry = (*map->entries)[idx];
    if (entry.valueType == NovaMapEntry::ValueType::String) {
        return static_cast<int64_t>(nova_value_from_string(entry.strValue));
    }
    if (entry.valueType == NovaMapEntry::ValueType::Object) {
        // numValue holds a void* object pointer; tag it so downstream
        // property access / unboxing works.
        return static_cast<int64_t>(nova_value_from_object(
            reinterpret_cast<void*>(static_cast<uintptr_t>(entry.numValue))));
    }
    return static_cast<int64_t>(nova_value_from_i64(entry.numValue));
}

int64_t nova_map_get_str_jsvalue(void* mapPtr, const char* key) {
    if (!mapPtr) return static_cast<int64_t>(nova::runtime::JS_VALUE_UNDEFINED);
    NovaMap* map = static_cast<NovaMap*>(mapPtr);
    int64_t idx = findEntry(map, NovaMapEntry::KeyType::String, 0, key);
    if (idx < 0) return static_cast<int64_t>(nova::runtime::JS_VALUE_UNDEFINED);
    const auto& entry = (*map->entries)[idx];
    if (entry.valueType == NovaMapEntry::ValueType::String) {
        return static_cast<int64_t>(nova_value_from_string(entry.strValue));
    }
    if (entry.valueType == NovaMapEntry::ValueType::Object) {
        return static_cast<int64_t>(nova_value_from_object(
            reinterpret_cast<void*>(static_cast<uintptr_t>(entry.numValue))));
    }
    return static_cast<int64_t>(nova_value_from_i64(entry.numValue));
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
                nova_array_push(
                    arr, static_cast<int64_t>(
                        reinterpret_cast<uintptr_t>(entry.strKey)));
            } else if (entry.keyType == NovaMapEntry::KeyType::Object) {
                nova_array_push(
                    arr, entry.numKey);
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
                nova_array_push(
                    arr, static_cast<int64_t>(
                        reinterpret_cast<uintptr_t>(entry.strValue)));
            } else if (entry.valueType == NovaMapEntry::ValueType::Object) {
                nova_array_push(arr, entry.numValue);
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
    if (!mapPtr || !callback) return;
    NovaMap* map = static_cast<NovaMap*>(mapPtr);
    if (!map->entries) return;

    // Cast callback to function pointer: callback(value, key, map)
    typedef void (*ForEachCallback)(int64_t, int64_t, void*);
    ForEachCallback fn = reinterpret_cast<ForEachCallback>(callback);

    for (auto& entry : *map->entries) {
        if (entry.deleted) continue;
        int64_t key = (entry.keyType == NovaMapEntry::KeyType::String)
            ? reinterpret_cast<int64_t>(entry.strKey) : entry.numKey;
        int64_t value = (entry.valueType == NovaMapEntry::ValueType::String)
            ? reinterpret_cast<int64_t>(entry.strValue) : entry.numValue;
        fn(value, key, mapPtr);
    }
}

// =========================================
// Map.groupBy (ES2024) - Static method
// Groups items by key returned from callback
// =========================================
void* nova_map_groupby(void* items_ptr, void* callback_ptr) {
    using GroupCallback = int64_t (*)(int64_t);
    GroupCallback callback = reinterpret_cast<GroupCallback>(callback_ptr);

    void* result = nova_map_create();
    if (!items_ptr || !callback) return result;

    // Treat items_ptr as a ValueArray metadata struct.
    char* metaBytes = static_cast<char*>(items_ptr);
    int64_t length = *reinterpret_cast<int64_t*>(metaBytes + 24);
    int64_t* elements = *reinterpret_cast<int64_t**>(metaBytes + 40);
    if (length <= 0 || !elements) return result;

    for (int64_t i = 0; i < length; ++i) {
        const int64_t element = elements[i];
        // Decide how to tag the element before invoking the callback.
        // Array literals of strings lower to i64(ptrtoint(ptr @.str to i64))
        // — those need the JS_VALUE_STRING_TAG so the callback's .length /
        // string operations resolve correctly. Numeric arrays store plain
        // i64 values; those wrap as doubles via nova_value_from_i64.
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
                    // strPtr points at the source array's stable string
                    // literal. `tmp.c_str()` would dangle before callback().
                    elementJs = nova_value_from_string(strPtr);
                    tagged = true;
                }
            }
            if (!tagged) {
                elementJs = nova_value_from_i64(element);
            }
        }
        const int64_t keyRaw = callback(static_cast<int64_t>(elementJs));

        // Decide key type: if the raw return looks like a heap pointer
        // (high value) and points to a printable C-string, treat as String.
        // Otherwise treat as Number. This mirrors nova_object_groupBy's
        // heuristic.
        bool keyIsString = false;
        std::string strKey;
        int64_t numKey = 0;
        const auto keyBits = static_cast<std::uint64_t>(keyRaw);
        if (nova::runtime::js_value_has_tag(
                keyBits, nova::runtime::JS_VALUE_STRING_TAG)) {
            keyIsString = true;
            const char* decoded = nova_value_to_string_ptr(keyBits);
            strKey = decoded ? decoded : "";
        } else if ((keyBits & nova::runtime::JS_VALUE_TAG_MASK) != 0 &&
                   keyRaw > (1LL << 31)) {
            numKey = static_cast<int64_t>(
                nova_value_to_number(keyBits));
        } else if (keyRaw > 0x10000) {
            const char* strPtr =
                reinterpret_cast<const char*>(static_cast<uintptr_t>(keyRaw));
            std::string tmp;
            bool ok = true;
            for (int k = 0; k < 64; ++k) {
                char c = strPtr[k];
                if (c == 0) break;
                if (c < 0x20 || c >= 0x7f) { ok = false; break; }
                tmp.push_back(c);
            }
            if (ok && !tmp.empty()) {
                keyIsString = true;
                strKey = tmp;
            } else {
                numKey = keyRaw;
            }
        } else {
            numKey = keyRaw;
        }

        // Find or create the group entry.
        NovaMap* map = static_cast<NovaMap*>(result);
        int64_t idx = keyIsString
            ? findEntry(map, NovaMapEntry::KeyType::String, 0, strKey.c_str())
            : findEntry(map, NovaMapEntry::KeyType::Number, numKey, nullptr);

        void* groupMeta = nullptr;
        if (idx >= 0) {
            groupMeta = reinterpret_cast<void*>(
                static_cast<uintptr_t>((*map->entries)[idx].numValue));
        } else {
            // Create a fresh ValueArray metadata for the new group using the
            // same machinery as nova_object_groupBy so .length / .join / etc.
            // work uniformly across Object.groupBy and Map.groupBy results.
            nova::runtime::ValueArray* groupArray =
                nova::runtime::create_value_array(8);
            groupArray->length = 0;
            groupMeta = nova::runtime::create_metadata_from_value_array(groupArray);

            NovaMapEntry entry;
            entry.deleted = false;
            entry.valueType = NovaMapEntry::ValueType::Object;
            entry.numValue = static_cast<int64_t>(
                reinterpret_cast<uintptr_t>(groupMeta));
            if (keyIsString) {
                entry.keyType = NovaMapEntry::KeyType::String;
                entry.strKey = strdup(strKey.c_str());
            } else {
                entry.keyType = NovaMapEntry::KeyType::Number;
                entry.numKey = numKey;
            }
            map->entries->push_back(entry);
            map->size++;
        }

        if (!groupMeta) continue;
        // Append `element` to the group's ValueArray.
        char* gMetaBytes = static_cast<char*>(groupMeta);
        int64_t gLen = *reinterpret_cast<int64_t*>(gMetaBytes + 24);
        int64_t gCap = *reinterpret_cast<int64_t*>(gMetaBytes + 32);
        int64_t** gElemPtrPtr = reinterpret_cast<int64_t**>(gMetaBytes + 40);
        if (gLen >= gCap) {
            // Grow the underlying buffer. The metadata struct's first
            // sizeof(ValueArray) bytes IS the ValueArray header, so we can
            // cast and call resize_value_array directly.
            nova::runtime::resize_value_array(
                reinterpret_cast<nova::runtime::ValueArray*>(groupMeta),
                std::max<int64_t>(gCap * 2, 8));
            gCap = *reinterpret_cast<int64_t*>(gMetaBytes + 32);
            gElemPtrPtr = reinterpret_cast<int64_t**>(gMetaBytes + 40);
        }
        int64_t* gElements = *gElemPtrPtr;
        gElements[gLen] = element;
        *reinterpret_cast<int64_t*>(gMetaBytes + 24) = gLen + 1;
    }
    return result;
}

}  // extern "C"
