#include "nova/runtime/Runtime.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <cstring>

namespace nova {
namespace runtime {

// Simple mark-and-sweep garbage collector
static bool gc_initialized = false;
static std::vector<void*> roots;
static std::unordered_set<ObjectHeader*> allocated_objects;

// FinalizationRegistry support (ES2021)
struct FinalizationEntry {
    void* target;           // The object being watched
    int64_t heldValue;      // Value passed to cleanup callback
    void* unregisterToken;  // Token for unregistering (optional)
};

struct FinalizationRegistry {
    void* callback;         // The cleanup callback function pointer
    std::vector<FinalizationEntry> entries;
};

static std::vector<FinalizationRegistry*> finalization_registries;
static std::vector<std::pair<void*, int64_t>> pending_finalizations; // callback, heldValue

void initialize_gc(size_t heap_size) {
    (void)heap_size; // Unused for now
    
    if (gc_initialized) return;
    
    roots.clear();
    allocated_objects.clear();
    gc_initialized = true;
}

void shutdown_gc() {
    if (!gc_initialized) return;
    
    // Free all allocated objects
    for (ObjectHeader* header : allocated_objects) {
        std::free(header);
    }
    
    roots.clear();
    allocated_objects.clear();
    gc_initialized = false;
}

void collect_garbage() {
    if (!gc_initialized) return;
    
    // Mark phase - mark all objects reachable from roots
    std::vector<ObjectHeader*> to_mark;
    
  // Add root objects to marking queue
    for (void* root : roots) {
        if (root) {
            ObjectHeader* header = reinterpret_cast<ObjectHeader*>(
                static_cast<char*>(root) - sizeof(ObjectHeader)
            );
            to_mark.push_back(header);
        }
    }
    
    // Mark all reachable objects
    while (!to_mark.empty()) {
        ObjectHeader* current = to_mark.back();
        to_mark.pop_back();
        
        if (current->is_marked) continue;
        
        current->is_marked = true;

        // Traverse object fields based on type
        TypeId type = static_cast<TypeId>(current->type_id);
        void* objPtr = reinterpret_cast<char*>(current) + sizeof(ObjectHeader);

        switch (type) {
            case TypeId::ARRAY: {
                // Array has elements pointer that may contain object references
                Array* arr = static_cast<Array*>(objPtr);
                if (arr->elements && arr->length > 0) {
                    void** elements = static_cast<void**>(arr->elements);
                    for (int64 i = 0; i < arr->length; i++) {
                        if (elements[i]) {
                            ObjectHeader* elemHeader = reinterpret_cast<ObjectHeader*>(
                                static_cast<char*>(elements[i]) - sizeof(ObjectHeader)
                            );
                            if (allocated_objects.find(elemHeader) != allocated_objects.end() &&
                                !elemHeader->is_marked) {
                                to_mark.push_back(elemHeader);
                            }
                        }
                    }
                }
                break;
            }
            case TypeId::OBJECT: {
                // Object has properties pointer (typically a Map-like structure)
                Object* obj = static_cast<Object*>(objPtr);
                if (obj->properties) {
                    ObjectHeader* propHeader = reinterpret_cast<ObjectHeader*>(
                        static_cast<char*>(obj->properties) - sizeof(ObjectHeader)
                    );
                    if (allocated_objects.find(propHeader) != allocated_objects.end() &&
                        !propHeader->is_marked) {
                        to_mark.push_back(propHeader);
                    }
                }
                break;
            }
            case TypeId::CLOSURE: {
                // Closure has environment pointer with captured variables
                Closure* closure = static_cast<Closure*>(objPtr);
                if (closure->environment) {
                    ObjectHeader* envHeader = reinterpret_cast<ObjectHeader*>(
                        static_cast<char*>(closure->environment) - sizeof(ObjectHeader)
                    );
                    if (allocated_objects.find(envHeader) != allocated_objects.end() &&
                        !envHeader->is_marked) {
                        to_mark.push_back(envHeader);
                    }
                }
                break;
            }
            case TypeId::STRING:
                // Strings have no object references (just char data)
                break;
            case TypeId::FUNCTION:
                // Functions have no object references
                break;
            default:
                // USER_DEFINED types - scan memory for potential pointers
                // This is conservative GC for unknown types
                break;
        }
    }
    
    // Sweep phase - deallocate unmarked objects
    auto it = allocated_objects.begin();
    while (it != allocated_objects.end()) {
        ObjectHeader* header = *it;
        if (!header->is_marked) {
            std::free(header);
            it = allocated_objects.erase(it);
        } else {
            header->is_marked = false; // Reset mark for next GC
            ++it;
        }
    }
}

void add_root(void* ptr) {
    if (!ptr) return;
    
    // Check if already a root
    if (std::find(roots.begin(), roots.end(), ptr) != roots.end()) {
        return;
    }
    
    roots.push_back(ptr);
    
    // Also track in allocated objects
    ObjectHeader* header = reinterpret_cast<ObjectHeader*>(
        static_cast<char*>(ptr) - sizeof(ObjectHeader)
    );
    allocated_objects.insert(header);
}

void remove_root(void* ptr) {
    if (!ptr) return;
    
    auto it = std::find(roots.begin(), roots.end(), ptr);
    if (it != roots.end()) {
        roots.erase(it);
    }
}

void register_object(void* ptr) {
    if (!ptr) return;
    
    ObjectHeader* header = reinterpret_cast<ObjectHeader*>(
        static_cast<char*>(ptr) - sizeof(ObjectHeader)
    );
    allocated_objects.insert(header);
}

void unregister_object(void* ptr) {
    if (!ptr) return;

    ObjectHeader* header = reinterpret_cast<ObjectHeader*>(
        static_cast<char*>(ptr) - sizeof(ObjectHeader)
    );
    allocated_objects.erase(header);
}

} // namespace runtime
} // namespace nova

// FinalizationRegistry C API (ES2021)
extern "C" {

// Create a new FinalizationRegistry with the given callback
void* nova_finalization_registry_create(void* callback) {
    auto* registry = new nova::runtime::FinalizationRegistry();
    registry->callback = callback;
    nova::runtime::finalization_registries.push_back(registry);
    return registry;
}

// Register a target object with the registry
void nova_finalization_registry_register(void* registryPtr, void* target, int64_t heldValue, void* unregisterToken) {
    if (!registryPtr || !target) return;

    auto* registry = static_cast<nova::runtime::FinalizationRegistry*>(registryPtr);

    nova::runtime::FinalizationEntry entry;
    entry.target = target;
    entry.heldValue = heldValue;
    entry.unregisterToken = unregisterToken;

    registry->entries.push_back(entry);
}

// Unregister entries with the given token
int64_t nova_finalization_registry_unregister(void* registryPtr, void* unregisterToken) {
    if (!registryPtr || !unregisterToken) return 0;

    auto* registry = static_cast<nova::runtime::FinalizationRegistry*>(registryPtr);

    int64_t removed = 0;
    auto it = registry->entries.begin();
    while (it != registry->entries.end()) {
        if (it->unregisterToken == unregisterToken) {
            it = registry->entries.erase(it);
            removed++;
        } else {
            ++it;
        }
    }

    return removed > 0 ? 1 : 0;  // Returns boolean: true if any were removed
}

// Check for finalized objects and queue callbacks (called during GC)
void nova_finalization_registry_check_and_cleanup() {
    // This is called after GC to check which registered objects were collected
    // For each collected object, we add the callback to pending_finalizations

    for (auto* registry : nova::runtime::finalization_registries) {
        auto it = registry->entries.begin();
        while (it != registry->entries.end()) {
            // Check if target is still alive (simple check - in real impl would check GC state)
            // For now, we don't actually detect GC'd objects - this would require integration
            // with the mark-sweep collector
            ++it;
        }
    }
}

// Run pending finalization callbacks
void nova_finalization_registry_run_pending() {
    // Execute all pending finalization callbacks
    for (auto& pair : nova::runtime::pending_finalizations) {
        // In a real implementation, we would call the callback with heldValue
        // For now, this is a placeholder
        (void)pair;
    }
    nova::runtime::pending_finalizations.clear();
}

// Cleanup all registries (called at shutdown)
void nova_finalization_registry_shutdown() {
    for (auto* registry : nova::runtime::finalization_registries) {
        delete registry;
    }
    nova::runtime::finalization_registries.clear();
    nova::runtime::pending_finalizations.clear();
}

// Manual cleanup trigger for testing - simulates object being collected
void nova_finalization_registry_cleanup_ref(void* registryPtr, void* target) {
    if (!registryPtr || !target) return;

    auto* registry = static_cast<nova::runtime::FinalizationRegistry*>(registryPtr);

    auto it = registry->entries.begin();
    while (it != registry->entries.end()) {
        if (it->target == target) {
            // Queue the finalization callback
            nova::runtime::pending_finalizations.push_back({registry->callback, it->heldValue});
            it = registry->entries.erase(it);
        } else {
            ++it;
        }
    }
}

} // extern "C"
