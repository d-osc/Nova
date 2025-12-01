// WeakSet Runtime Implementation for Nova Compiler
// ES2015 (ES6) WeakSet collection
// Values must be objects, held weakly (allow GC)

#include <cstdint>
#include <cstdlib>
#include <vector>

extern "C" {

// ============================================================================
// WeakSet Entry Structure
// ============================================================================

struct NovaWeakSetEntry {
    void* value;      // Object pointer (weak reference)
    bool deleted;
};

// ============================================================================
// WeakSet Structure
// ============================================================================

struct NovaWeakSet {
    std::vector<NovaWeakSetEntry>* entries;
};

// ============================================================================
// Helper: Find entry by value (object pointer)
// ============================================================================

static int64_t findWeakSetEntry(NovaWeakSet* set, void* value) {
    if (!set || !set->entries || !value) return -1;

    for (size_t i = 0; i < set->entries->size(); i++) {
        NovaWeakSetEntry& entry = (*set->entries)[i];
        if (!entry.deleted && entry.value == value) {
            return (int64_t)i;
        }
    }
    return -1;
}

// ============================================================================
// Constructor: new WeakSet()
// ============================================================================

void* nova_weakset_create() {
    NovaWeakSet* set = new NovaWeakSet();
    set->entries = new std::vector<NovaWeakSetEntry>();
    return set;
}

// ============================================================================
// WeakSet.prototype.add(value)
// Adds an object to the WeakSet
// Returns the WeakSet for chaining
// ============================================================================

void* nova_weakset_add(void* setPtr, void* value) {
    if (!setPtr || !value) return setPtr;
    NovaWeakSet* set = static_cast<NovaWeakSet*>(setPtr);

    // Check if value already exists
    int64_t idx = findWeakSetEntry(set, value);
    if (idx >= 0) {
        // Already exists, return set for chaining
        return setPtr;
    }

    // Add new entry
    NovaWeakSetEntry entry;
    entry.value = value;
    entry.deleted = false;
    set->entries->push_back(entry);

    return setPtr;
}

// ============================================================================
// WeakSet.prototype.has(value)
// Check if value exists in the WeakSet
// ============================================================================

int64_t nova_weakset_has(void* setPtr, void* value) {
    if (!setPtr || !value) return 0;
    NovaWeakSet* set = static_cast<NovaWeakSet*>(setPtr);

    return findWeakSetEntry(set, value) >= 0 ? 1 : 0;
}

// ============================================================================
// WeakSet.prototype.delete(value)
// Remove value from the WeakSet
// Returns true if value was deleted, false otherwise
// ============================================================================

int64_t nova_weakset_delete(void* setPtr, void* value) {
    if (!setPtr || !value) return 0;
    NovaWeakSet* set = static_cast<NovaWeakSet*>(setPtr);

    int64_t idx = findWeakSetEntry(set, value);
    if (idx >= 0) {
        (*set->entries)[idx].deleted = true;
        return 1;  // true
    }
    return 0;  // false
}

// ============================================================================
// Destructor helper
// ============================================================================

void nova_weakset_destroy(void* setPtr) {
    if (!setPtr) return;

    NovaWeakSet* set = static_cast<NovaWeakSet*>(setPtr);
    delete set->entries;
    delete set;
}

} // extern "C"
