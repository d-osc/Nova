/**
 * nova:domain - Domain Module Implementation
 *
 * Provides domain-based error handling for Nova programs.
 * Compatible with Node.js domain module.
 *
 * NOTE: This module is deprecated in Node.js but still available for compatibility.
 */

#include "nova/runtime/BuiltinModules.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#include <stack>

namespace nova {
namespace runtime {
namespace domain {

// Helper to allocate and copy string
static char* allocString(const std::string& str) {
    char* result = (char*)malloc(str.length() + 1);
    if (result) {
        strcpy(result, str.c_str());
    }
    return result;
}

// ============================================================================
// Domain Structure
// ============================================================================

struct Domain {
    int id;
    std::vector<void*> members;      // EventEmitters added to domain
    std::vector<void*> timers;       // Timers added to domain
    int disposed;
    void (*onError)(void* domain, void* error);
    void (*onDispose)(void* domain);
};

// Global domain stack
static std::stack<Domain*> domainStack;
static Domain* activeDomain = nullptr;
static int nextDomainId = 1;
static std::vector<Domain*> allDomains;

extern "C" {

// ============================================================================
// Module Functions
// ============================================================================

// Create a new domain
void* nova_domain_create() {
    Domain* domain = new Domain();
    domain->id = nextDomainId++;
    domain->disposed = 0;
    domain->onError = nullptr;
    domain->onDispose = nullptr;
    allDomains.push_back(domain);
    return domain;
}

// Get currently active domain (or nullptr)
void* nova_domain_active() {
    return activeDomain;
}

// ============================================================================
// Domain Properties
// ============================================================================

// Get domain ID
int nova_domain_id(void* domainPtr) {
    if (!domainPtr) return 0;
    return ((Domain*)domainPtr)->id;
}

// Get members count
int nova_domain_membersCount(void* domainPtr) {
    if (!domainPtr) return 0;
    Domain* domain = (Domain*)domainPtr;
    return (int)(domain->members.size() + domain->timers.size());
}

// Get member at index
void* nova_domain_getMember(void* domainPtr, int index) {
    if (!domainPtr) return nullptr;
    Domain* domain = (Domain*)domainPtr;

    int totalMembers = (int)domain->members.size();
    if (index < totalMembers) {
        return domain->members[index];
    }
    index -= totalMembers;
    if (index < (int)domain->timers.size()) {
        return domain->timers[index];
    }
    return nullptr;
}

// Check if disposed
int nova_domain_disposed(void* domainPtr) {
    if (!domainPtr) return 1;
    return ((Domain*)domainPtr)->disposed;
}

// ============================================================================
// Domain Methods
// ============================================================================

// Run a function within the domain
void* nova_domain_run(void* domainPtr, void* (*fn)(void*), void* arg) {
    if (!domainPtr || !fn) return nullptr;

    Domain* domain = (Domain*)domainPtr;
    if (domain->disposed) return nullptr;

    // Enter domain
    Domain* previousActive = activeDomain;
    activeDomain = domain;
    domainStack.push(domain);

    // Run the function
    void* result = fn(arg);

    // Exit domain
    if (!domainStack.empty()) {
        domainStack.pop();
    }
    activeDomain = previousActive;

    return result;
}

// Run function with error handling callback
void* nova_domain_runWithErrorHandler(
    void* domainPtr,
    void* (*fn)(void*),
    void* arg,
    void (*errorHandler)(void*, void*)
) {
    if (!domainPtr || !fn) return nullptr;

    Domain* domain = (Domain*)domainPtr;
    if (domain->disposed) return nullptr;

    // Set temporary error handler
    void (*prevHandler)(void*, void*) = domain->onError;
    domain->onError = errorHandler;

    // Enter domain
    Domain* previousActive = activeDomain;
    activeDomain = domain;
    domainStack.push(domain);

    // Run the function
    void* result = fn(arg);

    // Exit domain
    if (!domainStack.empty()) {
        domainStack.pop();
    }
    activeDomain = previousActive;
    domain->onError = prevHandler;

    return result;
}

// Add an EventEmitter to the domain
void nova_domain_add(void* domainPtr, void* emitter) {
    if (!domainPtr || !emitter) return;

    Domain* domain = (Domain*)domainPtr;
    if (domain->disposed) return;

    // Check if already added
    for (auto& m : domain->members) {
        if (m == emitter) return;
    }

    domain->members.push_back(emitter);
}

// Remove an EventEmitter from the domain
void nova_domain_remove(void* domainPtr, void* emitter) {
    if (!domainPtr || !emitter) return;

    Domain* domain = (Domain*)domainPtr;

    auto& members = domain->members;
    for (auto it = members.begin(); it != members.end(); ++it) {
        if (*it == emitter) {
            members.erase(it);
            return;
        }
    }

    // Also check timers
    auto& timers = domain->timers;
    for (auto it = timers.begin(); it != timers.end(); ++it) {
        if (*it == emitter) {
            timers.erase(it);
            return;
        }
    }
}

// Add a timer to the domain
void nova_domain_addTimer(void* domainPtr, void* timer) {
    if (!domainPtr || !timer) return;

    Domain* domain = (Domain*)domainPtr;
    if (domain->disposed) return;

    // Check if already added
    for (auto& t : domain->timers) {
        if (t == timer) return;
    }

    domain->timers.push_back(timer);
}

// Bind a callback to the domain
void* nova_domain_bind(void* domainPtr, void* callback) {
    if (!domainPtr || !callback) return callback;

    // In a full implementation, this would wrap the callback
    // to run within the domain context
    // For now, just return the callback
    return callback;
}

// Intercept - like bind but with error as first argument
void* nova_domain_intercept(void* domainPtr, void* callback) {
    if (!domainPtr || !callback) return callback;

    // In a full implementation, this would wrap the callback
    // to handle errors and run within domain context
    return callback;
}

// Enter the domain (explicitly)
void nova_domain_enter(void* domainPtr) {
    if (!domainPtr) return;

    Domain* domain = (Domain*)domainPtr;
    if (domain->disposed) return;

    activeDomain = domain;
    domainStack.push(domain);
}

// Exit the domain (explicitly)
void nova_domain_exit(void* domainPtr) {
    if (!domainPtr) return;

    Domain* domain = (Domain*)domainPtr;

    // Pop from stack if on top
    if (!domainStack.empty() && domainStack.top() == domain) {
        domainStack.pop();
    }

    // Update active domain
    if (!domainStack.empty()) {
        activeDomain = domainStack.top();
    } else {
        activeDomain = nullptr;
    }
}

// Dispose the domain (deprecated but available)
void nova_domain_dispose(void* domainPtr) {
    if (!domainPtr) return;

    Domain* domain = (Domain*)domainPtr;
    if (domain->disposed) return;

    domain->disposed = 1;

    // Clear members
    domain->members.clear();
    domain->timers.clear();

    // Exit domain if active
    nova_domain_exit(domainPtr);

    // Call dispose handler
    if (domain->onDispose) {
        domain->onDispose(domainPtr);
    }
}

// ============================================================================
// Event Handling
// ============================================================================

// Set error handler
void nova_domain_on_error(void* domainPtr, void (*handler)(void*, void*)) {
    if (!domainPtr) return;
    ((Domain*)domainPtr)->onError = handler;
}

// Set dispose handler
void nova_domain_on_dispose(void* domainPtr, void (*handler)(void*)) {
    if (!domainPtr) return;
    ((Domain*)domainPtr)->onDispose = handler;
}

// Emit error to domain
void nova_domain_emit_error(void* domainPtr, void* error) {
    if (!domainPtr) return;

    Domain* domain = (Domain*)domainPtr;
    if (domain->onError) {
        domain->onError(domainPtr, error);
    }
}

// ============================================================================
// EventEmitter-like Interface
// ============================================================================

// Generic event handler storage
struct DomainEventHandler {
    char* event;
    void (*callback)(void*, void*);
};

static std::vector<DomainEventHandler> domainEventHandlers;

// Register event handler
void nova_domain_on(void* domainPtr, const char* event, void* callback) {
    if (!domainPtr || !event || !callback) return;

    if (strcmp(event, "error") == 0) {
        nova_domain_on_error(domainPtr, (void (*)(void*, void*))callback);
    } else if (strcmp(event, "dispose") == 0) {
        nova_domain_on_dispose(domainPtr, (void (*)(void*))callback);
    }
}

// Register one-time event handler
void nova_domain_once(void* domainPtr, const char* event, void* callback) {
    // Simplified: same as on()
    nova_domain_on(domainPtr, event, callback);
}

// Remove event handler
void nova_domain_off(void* domainPtr, const char* event) {
    if (!domainPtr || !event) return;

    Domain* domain = (Domain*)domainPtr;

    if (strcmp(event, "error") == 0) {
        domain->onError = nullptr;
    } else if (strcmp(event, "dispose") == 0) {
        domain->onDispose = nullptr;
    }
}

// Emit event
void nova_domain_emit(void* domainPtr, const char* event, void* data) {
    if (!domainPtr || !event) return;

    if (strcmp(event, "error") == 0) {
        nova_domain_emit_error(domainPtr, data);
    } else if (strcmp(event, "dispose") == 0) {
        Domain* domain = (Domain*)domainPtr;
        if (domain->onDispose) {
            domain->onDispose(domainPtr);
        }
    }
}

// ============================================================================
// Utility Functions
// ============================================================================

// Check if emitter is in any domain
void* nova_domain_findDomainOf(void* emitter) {
    if (!emitter) return nullptr;

    for (auto& domain : allDomains) {
        if (domain->disposed) continue;

        for (auto& m : domain->members) {
            if (m == emitter) return domain;
        }
        for (auto& t : domain->timers) {
            if (t == emitter) return domain;
        }
    }

    return nullptr;
}

// Get domain stack depth
int nova_domain_stackDepth() {
    return (int)domainStack.size();
}

// Free domain
void nova_domain_free(void* domainPtr) {
    if (!domainPtr) return;

    Domain* domain = (Domain*)domainPtr;

    // Dispose if not already
    if (!domain->disposed) {
        nova_domain_dispose(domainPtr);
    }

    // Remove from global list
    for (auto it = allDomains.begin(); it != allDomains.end(); ++it) {
        if (*it == domain) {
            allDomains.erase(it);
            break;
        }
    }

    delete domain;
}

// Cleanup all domains
void nova_domain_cleanup() {
    // Clear domain stack
    while (!domainStack.empty()) {
        domainStack.pop();
    }
    activeDomain = nullptr;

    // Free all domains
    for (auto& domain : allDomains) {
        delete domain;
    }
    allDomains.clear();

    domainEventHandlers.clear();
}

// ============================================================================
// Deprecation Warning
// ============================================================================

// Get deprecation status
int nova_domain_isDeprecated() {
    return 1;  // domain module is deprecated
}

// Get deprecation message
char* nova_domain_deprecationMessage() {
    return allocString("The domain module is deprecated. Please use async_hooks or other error handling mechanisms.");
}

} // extern "C"

} // namespace domain
} // namespace runtime
} // namespace nova
