/**
 * nova:async_hooks - Async Hooks Module Implementation
 *
 * Provides async context tracking for Nova programs.
 * Compatible with Node.js async_hooks module.
 */

#include "nova/runtime/BuiltinModules.h"
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <atomic>
#include <map>
#include <vector>
#include <functional>

namespace nova {
namespace runtime {
namespace async_hooks {

// Helper to allocate and copy string
static char* allocString(const std::string& str) {
    char* result = (char*)malloc(str.length() + 1);
    if (result) {
        strcpy(result, str.c_str());
    }
    return result;
}

// ============================================================================
// Async ID Management
// ============================================================================

static std::atomic<int64_t> nextAsyncId(1);
static thread_local int64_t currentAsyncId = 0;
static thread_local int64_t currentTriggerAsyncId = 0;

// ============================================================================
// AsyncHook Structure
// ============================================================================

struct AsyncHook {
    int id;
    bool enabled;
    void (*init)(int64_t asyncId, const char* type, int64_t triggerAsyncId, void* resource);
    void (*before)(int64_t asyncId);
    void (*after)(int64_t asyncId);
    void (*destroy)(int64_t asyncId);
    void (*promiseResolve)(int64_t asyncId);
};

static std::vector<AsyncHook*> hooks;
static int nextHookId = 1;

// ============================================================================
// AsyncResource Structure
// ============================================================================

struct AsyncResource {
    int64_t asyncId;
    int64_t triggerAsyncId;
    char* type;
    bool destroyed;
};

static std::map<int64_t, AsyncResource*> resources;

// ============================================================================
// AsyncLocalStorage Structure
// ============================================================================

struct AsyncLocalStorage {
    int id;
    bool enabled;
    void* store;
};

static std::vector<AsyncLocalStorage*> localStorage;
static int nextStorageId = 1;

extern "C" {

// ============================================================================
// Core Functions
// ============================================================================

// Get current execution async ID
int64_t nova_async_hooks_executionAsyncId() {
    return currentAsyncId;
}

// Get trigger async ID
int64_t nova_async_hooks_triggerAsyncId() {
    return currentTriggerAsyncId;
}

// Get execution async resource
void* nova_async_hooks_executionAsyncResource() {
    auto it = resources.find(currentAsyncId);
    if (it != resources.end()) {
        return it->second;
    }
    return nullptr;
}

// ============================================================================
// AsyncHook Functions
// ============================================================================

// Create a new AsyncHook
void* nova_async_hooks_createHook(
    void (*init)(int64_t, const char*, int64_t, void*),
    void (*before)(int64_t),
    void (*after)(int64_t),
    void (*destroy)(int64_t),
    void (*promiseResolve)(int64_t)
) {
    AsyncHook* hook = new AsyncHook();
    hook->id = nextHookId++;
    hook->enabled = false;
    hook->init = init;
    hook->before = before;
    hook->after = after;
    hook->destroy = destroy;
    hook->promiseResolve = promiseResolve;
    hooks.push_back(hook);
    return hook;
}

// Enable an AsyncHook
void* nova_async_hooks_enable(void* hookPtr) {
    if (hookPtr) {
        AsyncHook* hook = (AsyncHook*)hookPtr;
        hook->enabled = true;
    }
    return hookPtr;
}

// Disable an AsyncHook
void* nova_async_hooks_disable(void* hookPtr) {
    if (hookPtr) {
        AsyncHook* hook = (AsyncHook*)hookPtr;
        hook->enabled = false;
    }
    return hookPtr;
}

// Get hook ID
int nova_async_hooks_hookAsyncId(void* hookPtr) {
    if (hookPtr) {
        return ((AsyncHook*)hookPtr)->id;
    }
    return 0;
}

// ============================================================================
// AsyncResource Functions
// ============================================================================

// Create a new AsyncResource
void* nova_async_hooks_AsyncResource_new(const char* type, int64_t triggerAsyncId) {
    AsyncResource* resource = new AsyncResource();
    resource->asyncId = nextAsyncId++;
    resource->triggerAsyncId = triggerAsyncId > 0 ? triggerAsyncId : currentAsyncId;
    resource->type = type ? allocString(type) : allocString("UNKNOWN");
    resource->destroyed = false;

    resources[resource->asyncId] = resource;

    // Call init hooks
    for (auto& hook : hooks) {
        if (hook->enabled && hook->init) {
            hook->init(resource->asyncId, resource->type, resource->triggerAsyncId, resource);
        }
    }

    return resource;
}

// Get async ID of resource
int64_t nova_async_hooks_AsyncResource_asyncId(void* resourcePtr) {
    if (resourcePtr) {
        return ((AsyncResource*)resourcePtr)->asyncId;
    }
    return 0;
}

// Get trigger async ID of resource
int64_t nova_async_hooks_AsyncResource_triggerAsyncId(void* resourcePtr) {
    if (resourcePtr) {
        return ((AsyncResource*)resourcePtr)->triggerAsyncId;
    }
    return 0;
}

// Run in async scope
void nova_async_hooks_AsyncResource_runInAsyncScope(void* resourcePtr, void (*fn)(void*), void* arg) {
    if (!resourcePtr || !fn) return;

    AsyncResource* resource = (AsyncResource*)resourcePtr;
    int64_t prevAsyncId = currentAsyncId;
    int64_t prevTriggerAsyncId = currentTriggerAsyncId;

    currentAsyncId = resource->asyncId;
    currentTriggerAsyncId = resource->triggerAsyncId;

    // Call before hooks
    for (auto& hook : hooks) {
        if (hook->enabled && hook->before) {
            hook->before(resource->asyncId);
        }
    }

    fn(arg);

    // Call after hooks
    for (auto& hook : hooks) {
        if (hook->enabled && hook->after) {
            hook->after(resource->asyncId);
        }
    }

    currentAsyncId = prevAsyncId;
    currentTriggerAsyncId = prevTriggerAsyncId;
}

// Emit destroy
void nova_async_hooks_AsyncResource_emitDestroy(void* resourcePtr) {
    if (!resourcePtr) return;

    AsyncResource* resource = (AsyncResource*)resourcePtr;
    if (resource->destroyed) return;

    resource->destroyed = true;

    // Call destroy hooks
    for (auto& hook : hooks) {
        if (hook->enabled && hook->destroy) {
            hook->destroy(resource->asyncId);
        }
    }
}

// Bind function to resource
void* nova_async_hooks_AsyncResource_bind(void* resourcePtr, void* fn) {
    (void)resourcePtr;
    return fn; // Simplified - just return the function
}

// ============================================================================
// AsyncLocalStorage Functions
// ============================================================================

// Create new AsyncLocalStorage
void* nova_async_hooks_AsyncLocalStorage_new() {
    AsyncLocalStorage* als = new AsyncLocalStorage();
    als->id = nextStorageId++;
    als->enabled = true;
    als->store = nullptr;
    localStorage.push_back(als);
    return als;
}

// Get store
void* nova_async_hooks_AsyncLocalStorage_getStore(void* alsPtr) {
    if (alsPtr) {
        AsyncLocalStorage* als = (AsyncLocalStorage*)alsPtr;
        if (als->enabled) {
            return als->store;
        }
    }
    return nullptr;
}

// Enter with store
void nova_async_hooks_AsyncLocalStorage_enterWith(void* alsPtr, void* store) {
    if (alsPtr) {
        AsyncLocalStorage* als = (AsyncLocalStorage*)alsPtr;
        als->store = store;
    }
}

// Run with store
void nova_async_hooks_AsyncLocalStorage_run(void* alsPtr, void* store, void (*fn)(void*), void* arg) {
    if (!alsPtr || !fn) return;

    AsyncLocalStorage* als = (AsyncLocalStorage*)alsPtr;
    void* prevStore = als->store;
    als->store = store;

    fn(arg);

    als->store = prevStore;
}

// Exit callback
void nova_async_hooks_AsyncLocalStorage_exit(void* alsPtr, void (*fn)(void*), void* arg) {
    if (!alsPtr || !fn) return;

    AsyncLocalStorage* als = (AsyncLocalStorage*)alsPtr;
    void* prevStore = als->store;
    als->store = nullptr;

    fn(arg);

    als->store = prevStore;
}

// Disable AsyncLocalStorage
void nova_async_hooks_AsyncLocalStorage_disable(void* alsPtr) {
    if (alsPtr) {
        AsyncLocalStorage* als = (AsyncLocalStorage*)alsPtr;
        als->enabled = false;
        als->store = nullptr;
    }
}

// ============================================================================
// Internal Trigger Functions (called by runtime)
// ============================================================================

// Trigger init
void nova_async_hooks_triggerInit(int64_t asyncId, const char* type, int64_t triggerAsyncId, void* resource) {
    for (auto& hook : hooks) {
        if (hook->enabled && hook->init) {
            hook->init(asyncId, type, triggerAsyncId, resource);
        }
    }
}

// Trigger before
void nova_async_hooks_triggerBefore(int64_t asyncId) {
    for (auto& hook : hooks) {
        if (hook->enabled && hook->before) {
            hook->before(asyncId);
        }
    }
}

// Trigger after
void nova_async_hooks_triggerAfter(int64_t asyncId) {
    for (auto& hook : hooks) {
        if (hook->enabled && hook->after) {
            hook->after(asyncId);
        }
    }
}

// Trigger destroy
void nova_async_hooks_triggerDestroy(int64_t asyncId) {
    for (auto& hook : hooks) {
        if (hook->enabled && hook->destroy) {
            hook->destroy(asyncId);
        }
    }
}

// Trigger promise resolve
void nova_async_hooks_triggerPromiseResolve(int64_t asyncId) {
    for (auto& hook : hooks) {
        if (hook->enabled && hook->promiseResolve) {
            hook->promiseResolve(asyncId);
        }
    }
}

// ============================================================================
// Utility Functions
// ============================================================================

// Generate new async ID
int64_t nova_async_hooks_newAsyncId() {
    return nextAsyncId++;
}

// Set current async context (for internal use)
void nova_async_hooks_setAsyncContext(int64_t asyncId, int64_t triggerId) {
    currentAsyncId = asyncId;
    currentTriggerAsyncId = triggerId;
}

// Get async ID type
char* nova_async_hooks_getAsyncIdType(int64_t asyncId) {
    auto it = resources.find(asyncId);
    if (it != resources.end()) {
        return allocString(it->second->type);
    }
    return allocString("UNKNOWN");
}

// Free resources
void nova_async_hooks_AsyncResource_free(void* resourcePtr) {
    if (resourcePtr) {
        AsyncResource* resource = (AsyncResource*)resourcePtr;
        if (!resource->destroyed) {
            nova_async_hooks_AsyncResource_emitDestroy(resourcePtr);
        }
        resources.erase(resource->asyncId);
        if (resource->type) free(resource->type);
        delete resource;
    }
}

void nova_async_hooks_AsyncHook_free(void* hookPtr) {
    if (hookPtr) {
        AsyncHook* hook = (AsyncHook*)hookPtr;
        hooks.erase(std::remove(hooks.begin(), hooks.end(), hook), hooks.end());
        delete hook;
    }
}

void nova_async_hooks_AsyncLocalStorage_free(void* alsPtr) {
    if (alsPtr) {
        AsyncLocalStorage* als = (AsyncLocalStorage*)alsPtr;
        localStorage.erase(std::remove(localStorage.begin(), localStorage.end(), als), localStorage.end());
        delete als;
    }
}

} // extern "C"

} // namespace async_hooks
} // namespace runtime
} // namespace nova
