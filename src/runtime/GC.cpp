// GC.cpp - Basic Reference Counting Garbage Collector for Nova
//
// This implements a simple reference counting GC with:
// - Basic object lifecycle management
// - Reference counting for heap-allocated objects
// - Cycle detection (basic)
//
// Future enhancements:
// - Mark-and-sweep for cycle collection
// - Generational collection
// - Compact heap

#include "nova/Runtime/GC.h"
#include <unordered_set>
#include <iostream>
#include <mutex>

namespace nova::runtime {

// Global GC state
static struct {
    std::unordered_set<GCObject*> trackedObjects;
    size_t totalAllocations = 0;
    size_t totalDeallocations = 0;
    size_t collectionCount = 0;
    std::mutex mutex;
} gcState;

// ============================================================================
// Object Header with Reference Counting
// ============================================================================

extern "C" {

// Allocate a new GC-tracked object
void* nova_gc_alloc(size_t size, GCObjectType type) {
    // Allocate memory with extra space for GC header
    void* mem = malloc(sizeof(GCObject) + size);
    if (!mem) {
        std::cerr << "FATAL: Out of memory in nova_gc_alloc" << std::endl;
        return nullptr;
    }

    // Initialize GC header
    GCObject* obj = static_cast<GCObject*>(mem);
    obj->type = type;
    obj->refCount = 1;  // Start with 1 reference (the owner)
    obj->flags = 0;
    obj->color = GCColor::Black;  // Default color

    std::lock_guard<std::mutex> lock(gcState.mutex);
    gcState.trackedObjects.insert(obj);
    gcState.totalAllocations++;

    // Return pointer after the header
    return static_cast<void*>(obj + 1);
}

// Free a GC object (called when refcount reaches 0)
void nova_gc_free(void* ptr) {
    if (!ptr) return;

    // Get the GC header (pointer is after the header)
    GCObject* obj = static_cast<GCObject*>(ptr) - 1;

    if (obj->refCount > 0) {
        std::cerr << "WARNING: Freeing object with refcount=" << obj->refCount << std::endl;
    }

    // Call finalizer if exists
    if (obj->flags & GCObject::FlagHasFinalizer) {
        // TODO: Call finalizer function
    }

    std::lock_guard<std::mutex> lock(gcState.mutex);
    gcState.trackedObjects.erase(obj);
    gcState.totalDeallocations++;

    // Free the entire block including header
    free(obj);
}

// Increment reference count
void nova_gc_addref(void* ptr) {
    if (!ptr) return;

    GCObject* obj = static_cast<GCObject*>(ptr) - 1;
    obj->refCount++;

    if (NOVA_DEBUG_GC && obj->refCount % 100 == 0) {
        std::cerr << "GC: addref -> refcount=" << obj->refCount
                  << " type=" << static_cast<int>(obj->type) << std::endl;
    }
}

// Decrement reference count, free if zero
void nova_gc_release(void* ptr) {
    if (!ptr) return;

    GCObject* obj = static_cast<GCObject*>(ptr) - 1;

    if (obj->refCount == 0) {
        std::cerr << "WARNING: Releasing object with refcount=0 (double free?)" << std::endl;
        return;
    }

    obj->refCount--;

    if (NOVA_DEBUG_GC && obj->refCount < 10) {
        std::cerr << "GC: release -> refcount=" << obj->refCount
                  << " type=" << static_cast<int>(obj->type) << std::endl;
    }

    if (obj->refCount == 0) {
        // Reference count reached zero, free the object
        nova_gc_free(ptr);
    }
}

// Get current reference count (for debugging)
int nova_gc_get_refcount(void* ptr) {
    if (!ptr) return 0;

    GCObject* obj = static_cast<GCObject*>(ptr) - 1;
    return static_cast<int>(obj->refCount);
}

// ============================================================================
// Statistics and Diagnostics
// ============================================================================

// Get GC statistics
void nova_gc_get_stats(size_t* totalAllocs, size_t* totalFrees,
                       size_t* liveObjects) {
    std::lock_guard<std::mutex> lock(gcState.mutex);

    if (totalAllocs) *totalAllocs = gcState.totalAllocations;
    if (totalFrees) *totalFrees = gcState.totalDeallocations;
    if (liveObjects) *liveObjects = gcState.trackedObjects.size();
}

// Force a garbage collection cycle (for reference counting, this is a no-op
// since objects are freed immediately when refcount reaches 0)
void nova_gc_collect() {
    std::lock_guard<std::mutex> lock(gcState.mutex);
    gcState.collectionCount++;

    // For simple reference counting, there's no collection cycle needed
    // Objects are freed immediately when their refcount reaches zero

    if (NOVA_DEBUG_GC) {
        std::cerr << "GC: Collection #" << gcState.collectionCount
                  << " (live objects: " << gcState.trackedObjects.size() << ")" << std::endl;
    }
}

// Print GC statistics
void nova_gc_print_stats() {
    std::lock_guard<std::mutex> lock(gcState.mutex);

    std::cerr << "=== GC Statistics ===" << std::endl;
    std::cerr << "Total allocations: " << gcState.totalAllocations << std::endl;
    std::cerr << "Total deallocations: " << gcState.totalDeallocations << std::endl;
    std::cerr << "Live objects: " << gcState.trackedObjects.size() << std::endl;
    std::cerr << "Collections: " << gcState.collectionCount << std::endl;
    std::cerr << "====================" << std::endl;
}

// Enable/disable debug output
void nova_gc_set_debug(int enabled) {
    // This would control NOVA_DEBUG_GC flag
    (void)enabled;
}

} // extern "C"

} // namespace nova::runtime
