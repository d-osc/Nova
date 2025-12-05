/**
 * nova:events - OPTIMIZED Events Module Implementation
 *
 * Performance optimizations:
 * 1. std::unordered_map instead of std::map (O(1) vs O(log n) lookup)
 * 2. Avoid vector copying in emit (use references)
 * 3. Reserve vector capacity to avoid reallocations
 * 4. Inline hot path functions
 * 5. Branch prediction hints
 * 6. Minimal allocations
 */

#include "nova/runtime/BuiltinModules.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

namespace nova {
namespace runtime {
namespace events {

// Helper to allocate and copy string
static inline char* allocString(const std::string& str) {
    char* result = (char*)malloc(str.length() + 1);
    if (result) {
        strcpy(result, str.c_str());
    }
    return result;
}

// ============================================================================
// Global Settings
// ============================================================================

static int defaultMaxListeners = 10;
static int captureRejections = 0;

// ============================================================================
// Listener Structure
// ============================================================================

typedef void (*ListenerCallback)(void* emitter, void* arg1, void* arg2, void* arg3);

struct Listener {
    ListenerCallback callback;
    int once;       // Remove after first call
    int prepend;    // Was added with prepend

    // Constructor for fast initialization
    inline Listener(ListenerCallback cb, int o, int p)
        : callback(cb), once(o), prepend(p) {}

    Listener() : callback(nullptr), once(0), prepend(0) {}
};

// ============================================================================
// EventEmitter Structure - OPTIMIZED
// ============================================================================

struct EventEmitter {
    int id;
    int maxListeners;
    int captureRejections;
    // OPTIMIZATION: Use unordered_map for O(1) average lookup instead of O(log n)
    std::unordered_map<std::string, std::vector<Listener>> events;
    void (*errorHandler)(void* emitter, void* error);
    void (*newListenerHandler)(void* emitter, const char* event, void* listener);
    void (*removeListenerHandler)(void* emitter, const char* event, void* listener);

    // Constructor to reserve initial capacity
    EventEmitter() : id(0), maxListeners(defaultMaxListeners),
                     captureRejections(captureRejections),
                     errorHandler(nullptr), newListenerHandler(nullptr),
                     removeListenerHandler(nullptr) {
        // Reserve capacity for common case (8 event types)
        events.reserve(8);
    }
};

static int nextEmitterId = 1;
static std::vector<EventEmitter*> allEmitters;

extern "C" {

// ============================================================================
// Module-level Functions
// ============================================================================

// Get default max listeners
int nova_events_getDefaultMaxListeners() {
    return defaultMaxListeners;
}

// Set default max listeners
void nova_events_setDefaultMaxListeners(int n) {
    if (n >= 0) {
        defaultMaxListeners = n;
    }
}

// Get capture rejections setting
int nova_events_getCaptureRejections() {
    return captureRejections;
}

// Set capture rejections
void nova_events_setCaptureRejections(int value) {
    captureRejections = value ? 1 : 0;
}

// ============================================================================
// EventEmitter Constructor
// ============================================================================

// Create new EventEmitter - OPTIMIZED
void* nova_events_EventEmitter_new() {
    EventEmitter* emitter = new EventEmitter();
    emitter->id = nextEmitterId++;
    allEmitters.push_back(emitter);
    return emitter;
}

// Free EventEmitter
void nova_events_EventEmitter_free(void* emitterPtr) {
    if (!emitterPtr) return;

    EventEmitter* emitter = (EventEmitter*)emitterPtr;

    // Remove from global list
    for (auto it = allEmitters.begin(); it != allEmitters.end(); ++it) {
        if (*it == emitter) {
            allEmitters.erase(it);
            break;
        }
    }

    delete emitter;
}

// ============================================================================
// EventEmitter Properties
// ============================================================================

// Get emitter ID
inline int nova_events_EventEmitter_id(void* emitterPtr) {
    if (!emitterPtr) [[unlikely]] return 0;
    return ((EventEmitter*)emitterPtr)->id;
}

// Get max listeners
inline int nova_events_EventEmitter_getMaxListeners(void* emitterPtr) {
    if (!emitterPtr) [[unlikely]] return defaultMaxListeners;
    return ((EventEmitter*)emitterPtr)->maxListeners;
}

// Set max listeners
void* nova_events_EventEmitter_setMaxListeners(void* emitterPtr, int n) {
    if (emitterPtr && n >= 0) [[likely]] {
        ((EventEmitter*)emitterPtr)->maxListeners = n;
    }
    return emitterPtr;
}

// ============================================================================
// Add Listeners - OPTIMIZED
// ============================================================================

// on(eventName, listener) - Add listener - OPTIMIZED
void* nova_events_EventEmitter_on(void* emitterPtr, const char* eventName, void* listener) {
    if (!emitterPtr || !eventName || !listener) [[unlikely]] return emitterPtr;

    EventEmitter* emitter = (EventEmitter*)emitterPtr;

    // Emit 'newListener' event
    if (emitter->newListenerHandler) [[unlikely]] {
        emitter->newListenerHandler(emitterPtr, eventName, listener);
    }

    // OPTIMIZATION: Single lookup and insert
    auto& listenerVec = emitter->events[eventName];

    // OPTIMIZATION: Reserve capacity for common case (avoid reallocation)
    if (listenerVec.capacity() == 0) {
        listenerVec.reserve(4);  // Reserve for 4 listeners initially
    }

    // OPTIMIZATION: Construct listener in-place
    listenerVec.emplace_back((ListenerCallback)listener, 0, 0);

    // Warn if exceeding max listeners
    if (emitter->maxListeners > 0 && (int)listenerVec.size() > emitter->maxListeners) [[unlikely]] {
        fprintf(stderr, "Warning: Possible EventEmitter memory leak detected. "
                "%d %s listeners added. Use emitter.setMaxListeners() to increase limit.\n",
                (int)listenerVec.size(), eventName);
    }

    return emitterPtr;
}

// addListener(eventName, listener) - Alias for on()
inline void* nova_events_EventEmitter_addListener(void* emitterPtr, const char* eventName, void* listener) {
    return nova_events_EventEmitter_on(emitterPtr, eventName, listener);
}

// once(eventName, listener) - Add one-time listener - OPTIMIZED
void* nova_events_EventEmitter_once(void* emitterPtr, const char* eventName, void* listener) {
    if (!emitterPtr || !eventName || !listener) [[unlikely]] return emitterPtr;

    EventEmitter* emitter = (EventEmitter*)emitterPtr;

    // Emit 'newListener' event
    if (emitter->newListenerHandler) [[unlikely]] {
        emitter->newListenerHandler(emitterPtr, eventName, listener);
    }

    // OPTIMIZATION: Single lookup, reserve, and emplace
    auto& listenerVec = emitter->events[eventName];
    if (listenerVec.capacity() == 0) {
        listenerVec.reserve(4);
    }
    listenerVec.emplace_back((ListenerCallback)listener, 1, 0);

    return emitterPtr;
}

// ============================================================================
// Emit Events - HIGHLY OPTIMIZED HOT PATH
// ============================================================================

// emit(eventName, ...args) - Emit event - HIGHLY OPTIMIZED
int nova_events_EventEmitter_emit(void* emitterPtr, const char* eventName, void* arg1, void* arg2, void* arg3) {
    if (!emitterPtr || !eventName) [[unlikely]] return 0;

    EventEmitter* emitter = (EventEmitter*)emitterPtr;

    // OPTIMIZATION: Single lookup for both error check and normal path
    auto it = emitter->events.find(eventName);
    if (it == emitter->events.end()) [[unlikely]] {
        // Handle 'error' event specially only if it's actually "error"
        if (strcmp(eventName, "error") == 0) [[unlikely]] {
            fprintf(stderr, "Unhandled 'error' event\n");
        }
        return 0;
    }

    auto& listeners = it->second;
    if (listeners.empty()) [[unlikely]] return 0;

    // OPTIMIZATION: Track once listeners count to avoid unnecessary work
    int onceCount = 0;

    // Call listeners WITHOUT copying the vector
    // We can do this because we'll remove once listeners after
    for (size_t i = 0; i < listeners.size(); ++i) {
        auto& l = listeners[i];
        if (l.callback) [[likely]] {
            l.callback(emitterPtr, arg1, arg2, arg3);
            if (l.once) onceCount++;
        }
    }

    // OPTIMIZATION: Only remove once listeners if there are any
    if (onceCount > 0) [[unlikely]] {
        listeners.erase(
            std::remove_if(listeners.begin(), listeners.end(),
                [](const Listener& l) { return l.once; }),
            listeners.end()
        );
    }

    return 1;
}

// Emit with single arg - OPTIMIZED
inline int nova_events_EventEmitter_emit1(void* emitterPtr, const char* eventName, void* arg) {
    return nova_events_EventEmitter_emit(emitterPtr, eventName, arg, nullptr, nullptr);
}

// Emit with no args - OPTIMIZED
inline int nova_events_EventEmitter_emit0(void* emitterPtr, const char* eventName) {
    return nova_events_EventEmitter_emit(emitterPtr, eventName, nullptr, nullptr, nullptr);
}

// ============================================================================
// Query Listeners - OPTIMIZED
// ============================================================================

// listenerCount(eventName) - Get number of listeners - OPTIMIZED
inline int nova_events_EventEmitter_listenerCount(void* emitterPtr, const char* eventName) {
    if (!emitterPtr || !eventName) [[unlikely]] return 0;

    EventEmitter* emitter = (EventEmitter*)emitterPtr;

    auto it = emitter->events.find(eventName);
    if (it == emitter->events.end()) [[unlikely]] return 0;

    return (int)it->second.size();
}

// eventNames() - Get array of event names
char** nova_events_EventEmitter_eventNames(void* emitterPtr, int* count) {
    if (!emitterPtr) {
        *count = 0;
        return nullptr;
    }

    EventEmitter* emitter = (EventEmitter*)emitterPtr;

    *count = (int)emitter->events.size();
    if (*count == 0) return nullptr;

    char** names = (char**)malloc(*count * sizeof(char*));
    int i = 0;
    for (auto& pair : emitter->events) {
        names[i++] = allocString(pair.first);
    }

    return names;
}

// listeners(eventName) - Get array of listeners - OPTIMIZED
void** nova_events_EventEmitter_listeners(void* emitterPtr, const char* eventName, int* count) {
    if (!emitterPtr || !eventName) [[unlikely]] {
        *count = 0;
        return nullptr;
    }

    EventEmitter* emitter = (EventEmitter*)emitterPtr;

    auto it = emitter->events.find(eventName);
    if (it == emitter->events.end()) [[unlikely]] {
        *count = 0;
        return nullptr;
    }

    *count = (int)it->second.size();
    if (*count == 0) [[unlikely]] return nullptr;

    void** listeners = (void**)malloc(*count * sizeof(void*));
    int i = 0;
    for (auto& l : it->second) {
        listeners[i++] = (void*)l.callback;
    }

    return listeners;
}

// rawListeners(eventName) - Get array of raw listeners
inline void** nova_events_EventEmitter_rawListeners(void* emitterPtr, const char* eventName, int* count) {
    return nova_events_EventEmitter_listeners(emitterPtr, eventName, count);
}

// ============================================================================
// Remove Listeners - OPTIMIZED
// ============================================================================

// off(eventName, listener) - Remove listener - OPTIMIZED
void* nova_events_EventEmitter_off(void* emitterPtr, const char* eventName, void* listener) {
    if (!emitterPtr || !eventName || !listener) [[unlikely]] return emitterPtr;

    EventEmitter* emitter = (EventEmitter*)emitterPtr;

    auto it = emitter->events.find(eventName);
    if (it == emitter->events.end()) [[unlikely]] return emitterPtr;

    auto& listeners = it->second;
    for (auto lit = listeners.begin(); lit != listeners.end(); ++lit) {
        if (lit->callback == (ListenerCallback)listener) [[likely]] {
            // Emit 'removeListener' event
            if (emitter->removeListenerHandler) [[unlikely]] {
                emitter->removeListenerHandler(emitterPtr, eventName, listener);
            }
            listeners.erase(lit);
            break;
        }
    }

    return emitterPtr;
}

// removeListener(eventName, listener) - Alias for off()
inline void* nova_events_EventEmitter_removeListener(void* emitterPtr, const char* eventName, void* listener) {
    return nova_events_EventEmitter_off(emitterPtr, eventName, listener);
}

// removeAllListeners([eventName]) - Remove all listeners - OPTIMIZED
void* nova_events_EventEmitter_removeAllListeners(void* emitterPtr, const char* eventName) {
    if (!emitterPtr) [[unlikely]] return emitterPtr;

    EventEmitter* emitter = (EventEmitter*)emitterPtr;

    if (eventName) [[likely]] {
        // Remove listeners for specific event
        auto it = emitter->events.find(eventName);
        if (it != emitter->events.end()) [[likely]] {
            // Emit 'removeListener' for each
            if (emitter->removeListenerHandler) [[unlikely]] {
                for (auto& l : it->second) {
                    emitter->removeListenerHandler(emitterPtr, eventName, (void*)l.callback);
                }
            }
            emitter->events.erase(it);
        }
    } else {
        // Remove all listeners for all events
        if (emitter->removeListenerHandler) [[unlikely]] {
            for (auto& pair : emitter->events) {
                for (auto& l : pair.second) {
                    emitter->removeListenerHandler(emitterPtr, pair.first.c_str(), (void*)l.callback);
                }
            }
        }
        emitter->events.clear();
    }

    return emitterPtr;
}

// ============================================================================
// Prepend Listeners - OPTIMIZED
// ============================================================================

// prependListener(eventName, listener) - Add listener to beginning - OPTIMIZED
void* nova_events_EventEmitter_prependListener(void* emitterPtr, const char* eventName, void* listener) {
    if (!emitterPtr || !eventName || !listener) [[unlikely]] return emitterPtr;

    EventEmitter* emitter = (EventEmitter*)emitterPtr;

    // Emit 'newListener' event
    if (emitter->newListenerHandler) [[unlikely]] {
        emitter->newListenerHandler(emitterPtr, eventName, listener);
    }

    auto& listenerVec = emitter->events[eventName];
    if (listenerVec.capacity() == 0) {
        listenerVec.reserve(4);
    }

    listenerVec.insert(listenerVec.begin(), Listener((ListenerCallback)listener, 0, 1));

    return emitterPtr;
}

// prependOnceListener(eventName, listener) - Add one-time listener to beginning - OPTIMIZED
void* nova_events_EventEmitter_prependOnceListener(void* emitterPtr, const char* eventName, void* listener) {
    if (!emitterPtr || !eventName || !listener) [[unlikely]] return emitterPtr;

    EventEmitter* emitter = (EventEmitter*)emitterPtr;

    // Emit 'newListener' event
    if (emitter->newListenerHandler) [[unlikely]] {
        emitter->newListenerHandler(emitterPtr, eventName, listener);
    }

    auto& listenerVec = emitter->events[eventName];
    if (listenerVec.capacity() == 0) {
        listenerVec.reserve(4);
    }

    listenerVec.insert(listenerVec.begin(), Listener((ListenerCallback)listener, 1, 1));

    return emitterPtr;
}

// ============================================================================
// Static Methods
// ============================================================================

// EventEmitter.listenerCount(emitter, eventName) - deprecated
inline int nova_events_listenerCount(void* emitterPtr, const char* eventName) {
    return nova_events_EventEmitter_listenerCount(emitterPtr, eventName);
}

// EventEmitter.getEventListeners(emitter, eventName)
inline void** nova_events_getEventListeners(void* emitterPtr, const char* eventName, int* count) {
    return nova_events_EventEmitter_listeners(emitterPtr, eventName, count);
}

// EventEmitter.getMaxListeners(emitter)
inline int nova_events_getMaxListeners(void* emitterPtr) {
    return nova_events_EventEmitter_getMaxListeners(emitterPtr);
}

// EventEmitter.setMaxListeners(n, ...emitters) - Set max for multiple
void nova_events_setMaxListeners(int n, void** emitters, int count) {
    if (n < 0) [[unlikely]] return;

    if (emitters && count > 0) [[likely]] {
        for (int i = 0; i < count; i++) {
            if (emitters[i]) [[likely]] {
                ((EventEmitter*)emitters[i])->maxListeners = n;
            }
        }
    } else {
        // Set default
        defaultMaxListeners = n;
    }
}

// ============================================================================
// Special Event Handlers
// ============================================================================

// Set handler for 'newListener' event
inline void nova_events_EventEmitter_onNewListener(void* emitterPtr, void* handler) {
    if (!emitterPtr) [[unlikely]] return;
    ((EventEmitter*)emitterPtr)->newListenerHandler =
        (void (*)(void*, const char*, void*))handler;
}

// Set handler for 'removeListener' event
inline void nova_events_EventEmitter_onRemoveListener(void* emitterPtr, void* handler) {
    if (!emitterPtr) [[unlikely]] return;
    ((EventEmitter*)emitterPtr)->removeListenerHandler =
        (void (*)(void*, const char*, void*))handler;
}

// Set handler for 'error' event
void nova_events_EventEmitter_onError(void* emitterPtr, void* handler) {
    if (!emitterPtr || !handler) [[unlikely]] return;

    EventEmitter* emitter = (EventEmitter*)emitterPtr;

    auto& errorListeners = emitter->events["error"];
    if (errorListeners.capacity() == 0) {
        errorListeners.reserve(4);
    }
    errorListeners.emplace_back((ListenerCallback)handler, 0, 0);
}

// ============================================================================
// Async Helpers
// ============================================================================

// events.once(emitter, name) - Returns promise-like (simplified)
void* nova_events_once(void* emitterPtr, const char* eventName) {
    // In a full implementation, this would return a Promise
    // For now, just set up a one-time listener
    // This is a placeholder for the async API
    (void)emitterPtr;
    (void)eventName;
    return nullptr;
}

// events.on(emitter, eventName) - Returns async iterator (simplified)
void* nova_events_on(void* emitterPtr, const char* eventName) {
    // In a full implementation, this would return an AsyncIterator
    // For now, just a placeholder
    (void)emitterPtr;
    (void)eventName;
    return nullptr;
}

// ============================================================================
// AbortSignal Support
// ============================================================================

// events.addAbortListener(signal, listener)
void* nova_events_addAbortListener(void* signal, void* listener) {
    // Add abort listener to signal
    // This would integrate with AbortController/AbortSignal
    if (!signal || !listener) [[unlikely]] return nullptr;

    // For now, just return a disposable object
    return listener;
}

// ============================================================================
// Error Monitor Symbol
// ============================================================================

// Get error monitor symbol
void* nova_events_errorMonitor() {
    // Return a unique identifier for error monitoring
    static int errorMonitorSymbol = 0xE4404;
    return &errorMonitorSymbol;
}

// ============================================================================
// Utility Functions
// ============================================================================

// Free event names array
void nova_events_freeEventNames(char** names, int count) {
    if (names) [[likely]] {
        for (int i = 0; i < count; i++) {
            if (names[i]) [[likely]] free(names[i]);
        }
        free(names);
    }
}

// Free listeners array
inline void nova_events_freeListeners(void** listeners) {
    if (listeners) [[likely]] {
        free(listeners);
    }
}

// Cleanup all emitters
void nova_events_cleanup() {
    for (auto& emitter : allEmitters) {
        delete emitter;
    }
    allEmitters.clear();
}

// ============================================================================
// EventTarget Interface (Web API compatibility)
// ============================================================================

// addEventListener (Web API style)
inline void nova_events_EventEmitter_addEventListener(void* emitterPtr, const char* type, void* listener, int options) {
    if (options & 1) [[unlikely]] {  // once option
        nova_events_EventEmitter_once(emitterPtr, type, listener);
    } else {
        nova_events_EventEmitter_on(emitterPtr, type, listener);
    }
}

// removeEventListener (Web API style)
inline void nova_events_EventEmitter_removeEventListener(void* emitterPtr, const char* type, void* listener) {
    nova_events_EventEmitter_off(emitterPtr, type, listener);
}

// dispatchEvent (Web API style)
inline int nova_events_EventEmitter_dispatchEvent(void* emitterPtr, const char* type, void* event) {
    return nova_events_EventEmitter_emit1(emitterPtr, type, event);
}

} // extern "C"

} // namespace events
} // namespace runtime
} // namespace nova
