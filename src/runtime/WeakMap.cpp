// WeakMap Runtime Implementation for Nova Compiler
// ES2015 (ES6) WeakMap collection
// Keys must be objects, held weakly (allow GC)

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

extern "C" {

// ============================================================================
// WeakMap Entry Structure
// ============================================================================

struct NovaWeakMapEntry {
    void* key;        // Object pointer (weak reference)
    int64_t numValue; // Value (number or pointer)
    bool isStringValue;
    char* strValue;   // String value if applicable
    bool deleted;
};

// ============================================================================
// WeakMap Structure
// ============================================================================

struct NovaWeakMap {
    std::vector<NovaWeakMapEntry>* entries;
};

// ============================================================================
// Helper: Find entry by key (object pointer)
// ============================================================================

static int64_t findWeakMapEntry(NovaWeakMap* map, void* key) {
    if (!map || !map->entries || !key) return -1;

    for (size_t i = 0; i < map->entries->size(); i++) {
        NovaWeakMapEntry& entry = (*map->entries)[i];
        if (!entry.deleted && entry.key == key) {
            return (int64_t)i;
        }
    }
    return -1;
}

// ============================================================================
// Constructor: new WeakMap()
// ============================================================================

void* nova_weakmap_create() {
    NovaWeakMap* map = new NovaWeakMap();
    map->entries = new std::vector<NovaWeakMapEntry>();
    return map;
}

// ============================================================================
// WeakMap.prototype.set(key, value) - Object key, Number value
// Returns the WeakMap for chaining
// ============================================================================

void* nova_weakmap_set_obj_num(void* mapPtr, void* key, int64_t value) {
    if (!mapPtr || !key) return mapPtr;
    NovaWeakMap* map = static_cast<NovaWeakMap*>(mapPtr);

    // Check if key exists
    int64_t idx = findWeakMapEntry(map, key);
    if (idx >= 0) {
        // Update existing entry
        NovaWeakMapEntry& entry = (*map->entries)[idx];
        if (entry.isStringValue && entry.strValue) {
            free(entry.strValue);
            entry.strValue = nullptr;
        }
        entry.numValue = value;
        entry.isStringValue = false;
    } else {
        // Add new entry
        NovaWeakMapEntry entry;
        entry.key = key;
        entry.numValue = value;
        entry.isStringValue = false;
        entry.strValue = nullptr;
        entry.deleted = false;
        map->entries->push_back(entry);
    }
    return mapPtr;
}

// ============================================================================
// WeakMap.prototype.set(key, value) - Object key, String value
// ============================================================================

void* nova_weakmap_set_obj_str(void* mapPtr, void* key, const char* value) {
    if (!mapPtr || !key) return mapPtr;
    NovaWeakMap* map = static_cast<NovaWeakMap*>(mapPtr);

    int64_t idx = findWeakMapEntry(map, key);
    if (idx >= 0) {
        NovaWeakMapEntry& entry = (*map->entries)[idx];
        if (entry.isStringValue && entry.strValue) {
            free(entry.strValue);
        }
        entry.strValue = value ? strdup(value) : nullptr;
        entry.isStringValue = true;
        entry.numValue = 0;
    } else {
        NovaWeakMapEntry entry;
        entry.key = key;
        entry.strValue = value ? strdup(value) : nullptr;
        entry.isStringValue = true;
        entry.numValue = 0;
        entry.deleted = false;
        map->entries->push_back(entry);
    }
    return mapPtr;
}

// ============================================================================
// WeakMap.prototype.set(key, value) - Object key, Object value
// ============================================================================

void* nova_weakmap_set_obj_obj(void* mapPtr, void* key, void* value) {
    if (!mapPtr || !key) return mapPtr;
    NovaWeakMap* map = static_cast<NovaWeakMap*>(mapPtr);

    int64_t idx = findWeakMapEntry(map, key);
    if (idx >= 0) {
        NovaWeakMapEntry& entry = (*map->entries)[idx];
        if (entry.isStringValue && entry.strValue) {
            free(entry.strValue);
            entry.strValue = nullptr;
        }
        entry.numValue = reinterpret_cast<int64_t>(value);
        entry.isStringValue = false;
    } else {
        NovaWeakMapEntry entry;
        entry.key = key;
        entry.numValue = reinterpret_cast<int64_t>(value);
        entry.isStringValue = false;
        entry.strValue = nullptr;
        entry.deleted = false;
        map->entries->push_back(entry);
    }
    return mapPtr;
}

// ============================================================================
// WeakMap.prototype.get(key) - Returns number value
// ============================================================================

int64_t nova_weakmap_get_num(void* mapPtr, void* key) {
    if (!mapPtr || !key) return 0;
    NovaWeakMap* map = static_cast<NovaWeakMap*>(mapPtr);

    int64_t idx = findWeakMapEntry(map, key);
    if (idx >= 0) {
        return (*map->entries)[idx].numValue;
    }
    return 0;  // undefined -> 0
}

// ============================================================================
// WeakMap.prototype.get(key) - Returns string value
// ============================================================================

const char* nova_weakmap_get_str(void* mapPtr, void* key) {
    if (!mapPtr || !key) return nullptr;
    NovaWeakMap* map = static_cast<NovaWeakMap*>(mapPtr);

    int64_t idx = findWeakMapEntry(map, key);
    if (idx >= 0 && (*map->entries)[idx].isStringValue) {
        return (*map->entries)[idx].strValue;
    }
    return nullptr;  // undefined
}

// ============================================================================
// WeakMap.prototype.get(key) - Returns object value
// ============================================================================

void* nova_weakmap_get_obj(void* mapPtr, void* key) {
    if (!mapPtr || !key) return nullptr;
    NovaWeakMap* map = static_cast<NovaWeakMap*>(mapPtr);

    int64_t idx = findWeakMapEntry(map, key);
    if (idx >= 0 && !(*map->entries)[idx].isStringValue) {
        return reinterpret_cast<void*>((*map->entries)[idx].numValue);
    }
    return nullptr;  // undefined
}

// ============================================================================
// WeakMap.prototype.has(key) - Check if key exists
// ============================================================================

int64_t nova_weakmap_has(void* mapPtr, void* key) {
    if (!mapPtr || !key) return 0;
    NovaWeakMap* map = static_cast<NovaWeakMap*>(mapPtr);

    return findWeakMapEntry(map, key) >= 0 ? 1 : 0;
}

// ============================================================================
// WeakMap.prototype.delete(key) - Remove entry by key
// Returns true if entry was deleted, false otherwise
// ============================================================================

int64_t nova_weakmap_delete(void* mapPtr, void* key) {
    if (!mapPtr || !key) return 0;
    NovaWeakMap* map = static_cast<NovaWeakMap*>(mapPtr);

    int64_t idx = findWeakMapEntry(map, key);
    if (idx >= 0) {
        NovaWeakMapEntry& entry = (*map->entries)[idx];
        if (entry.isStringValue && entry.strValue) {
            free(entry.strValue);
            entry.strValue = nullptr;
        }
        entry.deleted = true;
        return 1;  // true
    }
    return 0;  // false
}

// ============================================================================
// WeakMap destructor helper
// ============================================================================

void nova_weakmap_destroy(void* mapPtr) {
    if (!mapPtr) return;
    NovaWeakMap* map = static_cast<NovaWeakMap*>(mapPtr);

    // Free string values
    for (auto& entry : *map->entries) {
        if (!entry.deleted && entry.isStringValue && entry.strValue) {
            free(entry.strValue);
        }
    }

    delete map->entries;
    delete map;
}

} // extern "C"
