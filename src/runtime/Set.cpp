// Set.cpp - ES2015+ Set implementation for Nova
// Provides Set collection with unique values

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>
#include <unordered_set>
#include <functional>

extern "C" {

// Forward declarations
void* nova_value_array_create();
void nova_value_array_push(void* arr, void* value);
int64_t nova_value_array_length(void* arr);
void* nova_value_array_at(void* arr, int64_t index);
void* nova_create_string(const char* str);

// Custom hash function for void* values
struct NovaValueHash {
    size_t operator()(void* val) const {
        return std::hash<void*>()(val);
    }
};

// Set structure
struct NovaSet {
    std::vector<void*> values;  // Maintain insertion order
    std::unordered_set<void*, NovaValueHash> lookup;  // Fast lookup
};

// ============================================================================
// Constructor
// ============================================================================

// Create empty Set
void* nova_set_create() {
    NovaSet* set = new NovaSet();
    return set;
}

// Create Set from array/iterable
void* nova_set_create_from(void* iterable) {
    NovaSet* set = new NovaSet();

    if (iterable) {
        int64_t len = nova_value_array_length(iterable);
        for (int64_t i = 0; i < len; i++) {
            void* val = nova_value_array_at(iterable, i);
            if (set->lookup.find(val) == set->lookup.end()) {
                set->values.push_back(val);
                set->lookup.insert(val);
            }
        }
    }

    return set;
}

// ============================================================================
// Properties
// ============================================================================

// Get size
int64_t nova_set_size(void* setPtr) {
    if (!setPtr) return 0;
    NovaSet* set = static_cast<NovaSet*>(setPtr);
    return static_cast<int64_t>(set->values.size());
}

// ============================================================================
// Instance Methods
// ============================================================================

// add(value) - Add value to set, returns set for chaining
void* nova_set_add(void* setPtr, void* value) {
    if (!setPtr) return setPtr;
    NovaSet* set = static_cast<NovaSet*>(setPtr);

    if (set->lookup.find(value) == set->lookup.end()) {
        set->values.push_back(value);
        set->lookup.insert(value);
    }

    return setPtr;  // Return set for chaining
}

// has(value) - Check if value exists
int64_t nova_set_has(void* setPtr, void* value) {
    if (!setPtr) return 0;
    NovaSet* set = static_cast<NovaSet*>(setPtr);
    return set->lookup.find(value) != set->lookup.end() ? 1 : 0;
}

// delete(value) - Remove value, returns true if existed
int64_t nova_set_delete(void* setPtr, void* value) {
    if (!setPtr) return 0;
    NovaSet* set = static_cast<NovaSet*>(setPtr);

    auto it = set->lookup.find(value);
    if (it != set->lookup.end()) {
        set->lookup.erase(it);
        // Remove from values vector
        for (auto vit = set->values.begin(); vit != set->values.end(); ++vit) {
            if (*vit == value) {
                set->values.erase(vit);
                break;
            }
        }
        return 1;
    }
    return 0;
}

// clear() - Remove all values
void nova_set_clear(void* setPtr) {
    if (!setPtr) return;
    NovaSet* set = static_cast<NovaSet*>(setPtr);
    set->values.clear();
    set->lookup.clear();
}

// values() - Returns array of values
void* nova_set_values(void* setPtr) {
    void* result = nova_value_array_create();
    if (!setPtr) return result;

    NovaSet* set = static_cast<NovaSet*>(setPtr);
    for (void* val : set->values) {
        nova_value_array_push(result, val);
    }

    return result;
}

// keys() - Same as values() for Set
void* nova_set_keys(void* setPtr) {
    return nova_set_values(setPtr);
}

// entries() - Returns array of [value, value] pairs
void* nova_set_entries(void* setPtr) {
    void* result = nova_value_array_create();
    if (!setPtr) return result;

    NovaSet* set = static_cast<NovaSet*>(setPtr);
    for (void* val : set->values) {
        void* pair = nova_value_array_create();
        nova_value_array_push(pair, val);
        nova_value_array_push(pair, val);
        nova_value_array_push(result, pair);
    }

    return result;
}

// forEach(callback) - Execute callback for each value
// Note: In full implementation, callback would be a function pointer
void nova_set_forEach(void* setPtr, void* callback) {
    if (!setPtr || !callback) return;
    // Simplified - actual implementation needs function call support
}

// ============================================================================
// ES2025 Set Methods
// ============================================================================

// union(other) - Returns new Set with values from both sets
void* nova_set_union(void* setPtr, void* otherPtr) {
    NovaSet* result = static_cast<NovaSet*>(nova_set_create());

    if (setPtr) {
        NovaSet* set = static_cast<NovaSet*>(setPtr);
        for (void* val : set->values) {
            if (result->lookup.find(val) == result->lookup.end()) {
                result->values.push_back(val);
                result->lookup.insert(val);
            }
        }
    }

    if (otherPtr) {
        NovaSet* other = static_cast<NovaSet*>(otherPtr);
        for (void* val : other->values) {
            if (result->lookup.find(val) == result->lookup.end()) {
                result->values.push_back(val);
                result->lookup.insert(val);
            }
        }
    }

    return result;
}

// intersection(other) - Returns new Set with values in both sets
void* nova_set_intersection(void* setPtr, void* otherPtr) {
    NovaSet* result = static_cast<NovaSet*>(nova_set_create());

    if (!setPtr || !otherPtr) return result;

    NovaSet* set = static_cast<NovaSet*>(setPtr);
    NovaSet* other = static_cast<NovaSet*>(otherPtr);

    for (void* val : set->values) {
        if (other->lookup.find(val) != other->lookup.end()) {
            result->values.push_back(val);
            result->lookup.insert(val);
        }
    }

    return result;
}

// difference(other) - Returns new Set with values in this but not in other
void* nova_set_difference(void* setPtr, void* otherPtr) {
    NovaSet* result = static_cast<NovaSet*>(nova_set_create());

    if (!setPtr) return result;

    NovaSet* set = static_cast<NovaSet*>(setPtr);
    NovaSet* other = otherPtr ? static_cast<NovaSet*>(otherPtr) : nullptr;

    for (void* val : set->values) {
        if (!other || other->lookup.find(val) == other->lookup.end()) {
            result->values.push_back(val);
            result->lookup.insert(val);
        }
    }

    return result;
}

// symmetricDifference(other) - Returns new Set with values in either but not both
void* nova_set_symmetricDifference(void* setPtr, void* otherPtr) {
    NovaSet* result = static_cast<NovaSet*>(nova_set_create());

    NovaSet* set = setPtr ? static_cast<NovaSet*>(setPtr) : nullptr;
    NovaSet* other = otherPtr ? static_cast<NovaSet*>(otherPtr) : nullptr;

    // Add values from set that are not in other
    if (set) {
        for (void* val : set->values) {
            if (!other || other->lookup.find(val) == other->lookup.end()) {
                result->values.push_back(val);
                result->lookup.insert(val);
            }
        }
    }

    // Add values from other that are not in set
    if (other) {
        for (void* val : other->values) {
            if (!set || set->lookup.find(val) == set->lookup.end()) {
                if (result->lookup.find(val) == result->lookup.end()) {
                    result->values.push_back(val);
                    result->lookup.insert(val);
                }
            }
        }
    }

    return result;
}

// isSubsetOf(other) - Returns true if all values are in other
int64_t nova_set_isSubsetOf(void* setPtr, void* otherPtr) {
    if (!setPtr) return 1;  // Empty set is subset of any set
    if (!otherPtr) return 0;

    NovaSet* set = static_cast<NovaSet*>(setPtr);
    NovaSet* other = static_cast<NovaSet*>(otherPtr);

    for (void* val : set->values) {
        if (other->lookup.find(val) == other->lookup.end()) {
            return 0;
        }
    }

    return 1;
}

// isSupersetOf(other) - Returns true if contains all values from other
int64_t nova_set_isSupersetOf(void* setPtr, void* otherPtr) {
    if (!otherPtr) return 1;  // Any set is superset of empty set
    if (!setPtr) return 0;

    NovaSet* set = static_cast<NovaSet*>(setPtr);
    NovaSet* other = static_cast<NovaSet*>(otherPtr);

    for (void* val : other->values) {
        if (set->lookup.find(val) == set->lookup.end()) {
            return 0;
        }
    }

    return 1;
}

// isDisjointFrom(other) - Returns true if no values in common
int64_t nova_set_isDisjointFrom(void* setPtr, void* otherPtr) {
    if (!setPtr || !otherPtr) return 1;  // Empty sets are disjoint

    NovaSet* set = static_cast<NovaSet*>(setPtr);
    NovaSet* other = static_cast<NovaSet*>(otherPtr);

    for (void* val : set->values) {
        if (other->lookup.find(val) != other->lookup.end()) {
            return 0;
        }
    }

    return 1;
}

} // extern "C"
