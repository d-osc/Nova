// WeakRef Runtime Implementation for Nova Compiler
// ES2021 WeakRef - Weak references to objects

#include <cstdint>
#include <cstdlib>

extern "C" {

// ============================================================================
// WeakRef Structure
// ============================================================================

struct NovaWeakRef {
    void* target;      // Weak reference to target object
    bool isAlive;      // Whether the target is still reachable
};

// ============================================================================
// Constructor: new WeakRef(target)
// Creates a weak reference to the target object
// ============================================================================

void* nova_weakref_create(void* target) {
    NovaWeakRef* ref = new NovaWeakRef();
    ref->target = target;
    ref->isAlive = (target != nullptr);
    return ref;
}

// ============================================================================
// WeakRef.prototype.deref()
// Returns the target object if still alive, otherwise undefined (nullptr)
// ============================================================================

void* nova_weakref_deref(void* refPtr) {
    if (!refPtr) return nullptr;

    NovaWeakRef* ref = static_cast<NovaWeakRef*>(refPtr);

    // In a full GC implementation, we would check if target is still alive
    // For now, we return the target if it was set and isAlive is true
    if (ref->isAlive && ref->target) {
        return ref->target;
    }

    return nullptr;  // undefined - target was garbage collected
}

// ============================================================================
// Internal: Mark target as collected (called by GC)
// ============================================================================

void nova_weakref_clear(void* refPtr) {
    if (!refPtr) return;

    NovaWeakRef* ref = static_cast<NovaWeakRef*>(refPtr);
    ref->target = nullptr;
    ref->isAlive = false;
}

// ============================================================================
// Internal: Check if WeakRef is still alive
// ============================================================================

int64_t nova_weakref_is_alive(void* refPtr) {
    if (!refPtr) return 0;

    NovaWeakRef* ref = static_cast<NovaWeakRef*>(refPtr);
    return (ref->isAlive && ref->target) ? 1 : 0;
}

// ============================================================================
// Destructor helper
// ============================================================================

void nova_weakref_destroy(void* refPtr) {
    if (!refPtr) return;

    NovaWeakRef* ref = static_cast<NovaWeakRef*>(refPtr);
    delete ref;
}

} // extern "C"
