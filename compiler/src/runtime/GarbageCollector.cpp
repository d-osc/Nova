#include "nova/runtime/Runtime.h"
#include <vector>
#include <unordered_set>
#include <algorithm>

namespace nova {
namespace runtime {

// Simple mark-and-sweep garbage collector
static bool gc_initialized = false;
static std::vector<void*> roots;
static std::unordered_set<ObjectHeader*> allocated_objects;

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
        
        // TODO: Add references from this object to marking queue
        // This would require traversing the object's fields
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
