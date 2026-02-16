// GC.h - Garbage Collection Interface for Nova Runtime
//
// This provides a simple reference-counting garbage collector for
// heap-allocated objects in the Nova runtime.

#ifndef NOVA_RUNTIME_GC_H
#define NOVA_RUNTIME_GC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// GC Object Types
// ============================================================================

typedef enum {
    GCObjectType_Unknown = 0,
    GCObjectType_String,
    GCObjectType_Array,
    GCObjectType_Object,
    GCObjectType_Function,
    GCObjectType_Closure,
    GCObjectType_Buffer,
    GCObjectType_Custom
} GCObjectType;

// ============================================================================
// GC Object Header (placed before each allocated object)
// ============================================================================

typedef enum {
    GCColor_White = 0,    // Unvisited (for mark-and-sweep)
    GCColor_Gray = 1,     // Visited but children not scanned
    GCColor_Black = 2     // Visited and children scanned
} GCColor;

typedef struct GCObject {
    uint32_t refCount;        // Reference count
    GCObjectType type;        // Type of object
    uint8_t color;            // For mark-and-sweep algorithms
    uint8_t flags;            // Various flags
    uint16_t reserved;        // Reserved for future use

    // Flag bits
    enum {
        FlagHasFinalizer = 0x01,
        FlagIsImmutable = 0x02,
        FlagIsPermanent = 0x04  // Never collect
    };
} GCObject;

// Debug flag (can be defined at compile time)
#define NOVA_DEBUG_GC 0

// ============================================================================
// GC API Functions
// ============================================================================

// Allocate a new GC-tracked object
// Returns a pointer to the object data (after the GC header)
void* nova_gc_alloc(size_t size, GCObjectType type);

// Free a GC object (usually called automatically when refcount reaches 0)
void nova_gc_free(void* ptr);

// Increment reference count
void nova_gc_addref(void* ptr);

// Decrement reference count, frees object if count reaches 0
void nova_gc_release(void* ptr);

// Get current reference count (for debugging/testing)
int nova_gc_get_refcount(void* ptr);

// ============================================================================
// Statistics and Diagnostics
// ============================================================================

// Get GC statistics
void nova_gc_get_stats(size_t* totalAllocs, size_t* totalFrees,
                       size_t* liveObjects);

// Force a garbage collection cycle
// For reference counting, this is mostly a no-op but can be used
// for statistics reporting
void nova_gc_collect();

// Print GC statistics to stderr
void nova_gc_print_stats(void);

// Enable/disable debug output
void nova_gc_set_debug(int enabled);

// ============================================================================
// Convenience Macros
// ============================================================================

// Allocate a typed object with GC tracking
#define NOVA_GC_ALLOC(Type, gcType) \
    (Type*)nova_gc_alloc(sizeof(Type), gcType)

// Add reference (returns same pointer for convenience)
#define NOVA_ADDREF(ptr) \
    (nova_gc_addref(ptr), (ptr))

// Release reference (sets pointer to NULL after)
#define NOVA_RELEASE(ptr) \
    do { \
        nova_gc_release(ptr); \
        (ptr) = NULL; \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif // NOVA_RUNTIME_GC_H
