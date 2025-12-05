/**
 * nova:events - Events Module Implementation
 *
 * Provides EventEmitter class for Nova programs.
 * Compatible with Node.js events module.
 */

#include "nova/runtime/BuiltinModules.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

namespace nova {
namespace runtime {
namespace events {

// Helper to allocate and copy string
static char* allocString(const std::string& str) {
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
};

// ============================================================================
// EventEmitter Structure
// ============================================================================

struct EventEmitter {
    int id;
    int maxListeners;
    int captureRejections;
    std::map<std::string, std::vector<Listener>> events;
    void (*errorHandler)(void* emitter, void* error);
    void (*newListenerHandler)(void* emitter, const char* event, void* listener);
    void (*removeListenerHandler)(void* emitter, const char* event, void* listener);
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

// Create new EventEmitter
void* nova_events_EventEmitter_new() {
    EventEmitter* emitter = new EventEmitter();
    emitter->id = nextEmitterId++;
    emitter->maxListeners = defaultMaxListeners;
    emitter->captureRejections = captureRejections;
    emitter->errorHandler = nullptr;
    emitter->newListenerHandler = nullptr;
    emitter->removeListenerHandler = nullptr;
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
int nova_events_EventEmitter_id(void* emitterPtr) {
    if (!emitterPtr) return 0;
    return ((EventEmitter*)emitterPtr)->id;
}

// Get max listeners
int nova_events_EventEmitter_getMaxListeners(void* emitterPtr) {
    if (!emitterPtr) return defaultMaxListeners;
    return ((EventEmitter*)emitterPtr)->maxListeners;
}

// Set max listeners
void* nova_events_EventEmitter_setMaxListeners(void* emitterPtr, int n) {
    if (emitterPtr && n >= 0) {
        ((EventEmitter*)emitterPtr)->maxListeners = n;
    }
    return emitterPtr;
}

// ============================================================================
// Add Listeners
// ============================================================================

// on(eventName, listener) - Add listener
void* nova_events_EventEmitter_on(void* emitterPtr, const char* eventName, void* listener) {
    if (!emitterPtr || !eventName || !listener) return emitterPtr;

    EventEmitter* emitter = (EventEmitter*)emitterPtr;

    // Emit 'newListener' event
    if (emitter->newListenerHandler) {
        emitter->newListenerHandler(emitterPtr, eventName, listener);
    }

    Listener l;
    l.callback = (ListenerCallback)listener;
    l.once = 0;
    l.prepend = 0;

    emitter->events[eventName].push_back(l);

    // Warn if exceeding max listeners
    int count = (int)emitter->events[eventName].size();
    if (emitter->maxListeners > 0 && count > emitter->maxListeners) {
        fprintf(stderr, "Warning: Possible EventEmitter memory leak detected. "
                "%d %s listeners added. Use emitter.setMaxListeners() to increase limit.\n",
                count, eventName);
    }

    return emitterPtr;
}

// addListener(eventName, listener) - Alias for on()
void* nova_events_EventEmitter_addListener(void* emitterPtr, const char* eventName, void* listener) {
    return nova_events_EventEmitter_on(emitterPtr, eventName, listener);
}

// once(eventName, listener) - Add one-time listener
void* nova_events_EventEmitter_once(void* emitterPtr, const char* eventName, void* listener) {
    if (!emitterPtr || !eventName || !listener) return emitterPtr;

    EventEmitter* emitter = (EventEmitter*)emitterPtr;

    // Emit 'newListener' event
    if (emitter->newListenerHandler) {
        emitter->newListenerHandler(emitterPtr, eventName, listener);
    }

    Listener l;
    l.callback = (ListenerCallback)listener;
    l.once = 1;
    l.prepend = 0;

    emitter->events[eventName].push_back(l);

    return emitterPtr;
}

// prependListener(eventName, listener) - Add listener to beginning
void* nova_events_EventEmitter_prependListener(void* emitterPtr, const char* eventName, void* listener) {
    if (!emitterPtr || !eventName || !listener) return emitterPtr;

    EventEmitter* emitter = (EventEmitter*)emitterPtr;

    // Emit 'newListener' event
    if (emitter->newListenerHandler) {
        emitter->newListenerHandler(emitterPtr, eventName, listener);
    }

    Listener l;
    l.callback = (ListenerCallback)listener;
    l.once = 0;
    l.prepend = 1;

    emitter->events[eventName].insert(emitter->events[eventName].begin(), l);

    return emitterPtr;
}

// prependOnceListener(eventName, listener) - Add one-time listener to beginning
void* nova_events_EventEmitter_prependOnceListener(void* emitterPtr, const char* eventName, void* listener) {
    if (!emitterPtr || !eventName || !listener) return emitterPtr;

    EventEmitter* emitter = (EventEmitter*)emitterPtr;

    // Emit 'newListener' event
    if (emitter->newListenerHandler) {
        emitter->newListenerHandler(emitterPtr, eventName, listener);
    }

    Listener l;
    l.callback = (ListenerCallback)listener;
    l.once = 1;
    l.prepend = 1;

    emitter->events[eventName].insert(emitter->events[eventName].begin(), l);

    return emitterPtr;
}

// ============================================================================
// Remove Listeners
// ============================================================================

// off(eventName, listener) - Remove listener
void* nova_events_EventEmitter_off(void* emitterPtr, const char* eventName, void* listener) {
    if (!emitterPtr || !eventName || !listener) return emitterPtr;

    EventEmitter* emitter = (EventEmitter*)emitterPtr;

    auto it = emitter->events.find(eventName);
    if (it == emitter->events.end()) return emitterPtr;

    auto& listeners = it->second;
    for (auto lit = listeners.begin(); lit != listeners.end(); ++lit) {
        if (lit->callback == (ListenerCallback)listener) {
            // Emit 'removeListener' event
            if (emitter->removeListenerHandler) {
                emitter->removeListenerHandler(emitterPtr, eventName, listener);
            }
            listeners.erase(lit);
            break;
        }
    }

    return emitterPtr;
}

// removeListener(eventName, listener) - Alias for off()
void* nova_events_EventEmitter_removeListener(void* emitterPtr, const char* eventName, void* listener) {
    return nova_events_EventEmitter_off(emitterPtr, eventName, listener);
}

// removeAllListeners([eventName]) - Remove all listeners
void* nova_events_EventEmitter_removeAllListeners(void* emitterPtr, const char* eventName) {
    if (!emitterPtr) return emitterPtr;

    EventEmitter* emitter = (EventEmitter*)emitterPtr;

    if (eventName) {
        // Remove listeners for specific event
        auto it = emitter->events.find(eventName);
        if (it != emitter->events.end()) {
            // Emit 'removeListener' for each
            if (emitter->removeListenerHandler) {
                for (auto& l : it->second) {
                    emitter->removeListenerHandler(emitterPtr, eventName, (void*)l.callback);
                }
            }
            emitter->events.erase(it);
        }
    } else {
        // Remove all listeners for all events
        if (emitter->removeListenerHandler) {
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
// Emit Events
// ============================================================================

// emit(eventName, ...args) - Emit event
int nova_events_EventEmitter_emit(void* emitterPtr, const char* eventName, void* arg1, void* arg2, void* arg3) {
    if (!emitterPtr || !eventName) return 0;

    EventEmitter* emitter = (EventEmitter*)emitterPtr;

    // Handle 'error' event specially
    if (strcmp(eventName, "error") == 0) {
        auto it = emitter->events.find("error");
        if (it == emitter->events.end() || it->second.empty()) {
            // No error handler - print error and return
            fprintf(stderr, "Unhandled 'error' event\n");
            return 0;
        }
    }

    auto it = emitter->events.find(eventName);
    if (it == emitter->events.end()) return 0;

    // Copy listeners (in case they modify during emit)
    std::vector<Listener> listeners = it->second;

    // Remove once listeners from original
    auto& origListeners = it->second;
    origListeners.erase(
        std::remove_if(origListeners.begin(), origListeners.end(),
            [](const Listener& l) { return l.once; }),
        origListeners.end()
    );

    // Call listeners
    for (auto& l : listeners) {
        if (l.callback) {
            l.callback(emitterPtr, arg1, arg2, arg3);
        }
    }

    return listeners.empty() ? 0 : 1;
}

// Emit with single arg
int nova_events_EventEmitter_emit1(void* emitterPtr, const char* eventName, void* arg) {
    return nova_events_EventEmitter_emit(emitterPtr, eventName, arg, nullptr, nullptr);
}

// Emit with no args
int nova_events_EventEmitter_emit0(void* emitterPtr, const char* eventName) {
    return nova_events_EventEmitter_emit(emitterPtr, eventName, nullptr, nullptr, nullptr);
}

// ============================================================================
// Query Listeners
// ============================================================================

// listenerCount(eventName) - Get number of listeners
int nova_events_EventEmitter_listenerCount(void* emitterPtr, const char* eventName) {
    if (!emitterPtr || !eventName) return 0;

    EventEmitter* emitter = (EventEmitter*)emitterPtr;

    auto it = emitter->events.find(eventName);
    if (it == emitter->events.end()) return 0;

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

// listeners(eventName) - Get array of listeners
void** nova_events_EventEmitter_listeners(void* emitterPtr, const char* eventName, int* count) {
    if (!emitterPtr || !eventName) {
        *count = 0;
        return nullptr;
    }

    EventEmitter* emitter = (EventEmitter*)emitterPtr;

    auto it = emitter->events.find(eventName);
    if (it == emitter->events.end()) {
        *count = 0;
        return nullptr;
    }

    *count = (int)it->second.size();
    if (*count == 0) return nullptr;

    void** listeners = (void**)malloc(*count * sizeof(void*));
    int i = 0;
    for (auto& l : it->second) {
        listeners[i++] = (void*)l.callback;
    }

    return listeners;
}

// rawListeners(eventName) - Get array of raw listeners (includes wrapper info)
void** nova_events_EventEmitter_rawListeners(void* emitterPtr, const char* eventName, int* count) {
    // Same as listeners for our implementation
    return nova_events_EventEmitter_listeners(emitterPtr, eventName, count);
}

// ============================================================================
// Static Methods
// ============================================================================

// EventEmitter.listenerCount(emitter, eventName) - deprecated
int nova_events_listenerCount(void* emitterPtr, const char* eventName) {
    return nova_events_EventEmitter_listenerCount(emitterPtr, eventName);
}

// EventEmitter.getEventListeners(emitter, eventName)
void** nova_events_getEventListeners(void* emitterPtr, const char* eventName, int* count) {
    return nova_events_EventEmitter_listeners(emitterPtr, eventName, count);
}

// EventEmitter.getMaxListeners(emitter)
int nova_events_getMaxListeners(void* emitterPtr) {
    return nova_events_EventEmitter_getMaxListeners(emitterPtr);
}

// EventEmitter.setMaxListeners(n, ...emitters) - Set max for multiple
void nova_events_setMaxListeners(int n, void** emitters, int count) {
    if (n < 0) return;

    if (emitters && count > 0) {
        for (int i = 0; i < count; i++) {
            if (emitters[i]) {
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
void nova_events_EventEmitter_onNewListener(void* emitterPtr, void* handler) {
    if (!emitterPtr) return;
    ((EventEmitter*)emitterPtr)->newListenerHandler =
        (void (*)(void*, const char*, void*))handler;
}

// Set handler for 'removeListener' event
void nova_events_EventEmitter_onRemoveListener(void* emitterPtr, void* handler) {
    if (!emitterPtr) return;
    ((EventEmitter*)emitterPtr)->removeListenerHandler =
        (void (*)(void*, const char*, void*))handler;
}

// Set handler for 'error' event
void nova_events_EventEmitter_onError(void* emitterPtr, void* handler) {
    if (!emitterPtr || !handler) return;

    EventEmitter* emitter = (EventEmitter*)emitterPtr;

    Listener l;
    l.callback = (ListenerCallback)handler;
    l.once = 0;
    l.prepend = 0;

    emitter->events["error"].push_back(l);
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
    if (!signal || !listener) return nullptr;

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
    if (names) {
        for (int i = 0; i < count; i++) {
            if (names[i]) free(names[i]);
        }
        free(names);
    }
}

// Free listeners array
void nova_events_freeListeners(void** listeners) {
    if (listeners) {
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
void nova_events_EventEmitter_addEventListener(void* emitterPtr, const char* type, void* listener, int options) {
    if (options & 1) {  // once option
        nova_events_EventEmitter_once(emitterPtr, type, listener);
    } else {
        nova_events_EventEmitter_on(emitterPtr, type, listener);
    }
}

// removeEventListener (Web API style)
void nova_events_EventEmitter_removeEventListener(void* emitterPtr, const char* type, void* listener) {
    nova_events_EventEmitter_off(emitterPtr, type, listener);
}

// dispatchEvent (Web API style)
int nova_events_EventEmitter_dispatchEvent(void* emitterPtr, const char* type, void* event) {
    return nova_events_EventEmitter_emit1(emitterPtr, type, event);
}

} // extern "C"

} // namespace events
} // namespace runtime
} // namespace nova
