// Nova Runtime - Promise Implementation (ES2015+)
// JavaScript-like Promise for async/await support

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

extern "C" {

// Forward declarations
void nova_console_log_string(const char* str);
void nova_console_error_string(const char* str);

// Forward declarations for Promise functions (needed for mutual recursion)
void nova_promise_fulfill(void* promisePtr, int64_t value);
void nova_promise_reject_internal(void* promisePtr, int64_t reason);

// ============================================================================
// Promise State
// ============================================================================
enum class PromiseState {
    PENDING,
    FULFILLED,
    REJECTED
};

// ============================================================================
// Callback entry for then/catch/finally
// ============================================================================
struct PromiseCallback {
    enum class Type {
        THEN,
        CATCH,
        FINALLY
    };

    Type type;
    void* callback;      // Function pointer
    void* nextPromise;   // Promise to chain result to
};

// ============================================================================
// Promise Structure
// ============================================================================
struct NovaPromise {
    PromiseState state;
    int64_t value;           // Fulfilled value (for simplicity, using int64_t)
    int64_t error;           // Rejection reason
    std::vector<PromiseCallback> callbacks;
    std::mutex mutex;
    std::condition_variable cv;
    bool hasValue;
    bool hasError;
};

// ============================================================================
// Microtask Queue (for proper Promise scheduling)
// ============================================================================
static std::queue<std::function<void()>> microtaskQueue;
static std::mutex microtaskMutex;
static std::atomic<bool> processingMicrotasks{false};

void nova_promise_queue_microtask(std::function<void()> task) {
    std::lock_guard<std::mutex> lock(microtaskMutex);
    microtaskQueue.push(task);
}

void nova_promise_process_microtasks() {
    if (processingMicrotasks.exchange(true)) {
        return; // Already processing
    }

    while (true) {
        std::function<void()> task;
        {
            std::lock_guard<std::mutex> lock(microtaskMutex);
            if (microtaskQueue.empty()) {
                processingMicrotasks.store(false);
                return;
            }
            task = microtaskQueue.front();
            microtaskQueue.pop();
        }
        task();
    }
}

// ============================================================================
// Promise Creation
// ============================================================================

// new Promise((resolve, reject) => ...)
// For simplicity, we create a pending promise and provide resolve/reject functions
void* nova_promise_create() {
    NovaPromise* promise = new NovaPromise();
    promise->state = PromiseState::PENDING;
    promise->value = 0;
    promise->error = 0;
    promise->hasValue = false;
    promise->hasError = false;
    return promise;
}

// Promise.resolve(value) - Create an already-fulfilled promise
void* nova_promise_resolve(int64_t value) {
    NovaPromise* promise = new NovaPromise();
    promise->state = PromiseState::FULFILLED;
    promise->value = value;
    promise->error = 0;
    promise->hasValue = true;
    promise->hasError = false;
    return promise;
}

// Promise.reject(reason) - Create an already-rejected promise
void* nova_promise_reject(int64_t reason) {
    NovaPromise* promise = new NovaPromise();
    promise->state = PromiseState::REJECTED;
    promise->value = 0;
    promise->error = reason;
    promise->hasValue = false;
    promise->hasError = true;
    return promise;
}

// ============================================================================
// Promise Resolution
// ============================================================================

// Internal: Process callbacks when promise settles
void nova_promise_process_callbacks(NovaPromise* promise) {
    typedef int64_t (*ThenCallback)(int64_t);
    typedef int64_t (*CatchCallback)(int64_t);
    typedef void (*FinallyCallback)();

    for (auto& cb : promise->callbacks) {
        NovaPromise* nextPromise = static_cast<NovaPromise*>(cb.nextPromise);

        switch (cb.type) {
            case PromiseCallback::Type::THEN:
                if (promise->state == PromiseState::FULFILLED && cb.callback) {
                    try {
                        int64_t result = reinterpret_cast<ThenCallback>(cb.callback)(promise->value);
                        if (nextPromise) {
                            nova_promise_fulfill(nextPromise, result);
                        }
                    } catch (...) {
                        if (nextPromise) {
                            nova_promise_reject_internal(nextPromise, -1);
                        }
                    }
                } else if (promise->state == PromiseState::REJECTED && nextPromise) {
                    // Pass rejection to next promise
                    nova_promise_reject_internal(nextPromise, promise->error);
                }
                break;

            case PromiseCallback::Type::CATCH:
                if (promise->state == PromiseState::REJECTED && cb.callback) {
                    try {
                        int64_t result = reinterpret_cast<CatchCallback>(cb.callback)(promise->error);
                        if (nextPromise) {
                            nova_promise_fulfill(nextPromise, result);
                        }
                    } catch (...) {
                        if (nextPromise) {
                            nova_promise_reject_internal(nextPromise, -1);
                        }
                    }
                } else if (promise->state == PromiseState::FULFILLED && nextPromise) {
                    // Pass fulfillment to next promise
                    nova_promise_fulfill(nextPromise, promise->value);
                }
                break;

            case PromiseCallback::Type::FINALLY:
                if (cb.callback) {
                    reinterpret_cast<FinallyCallback>(cb.callback)();
                }
                // Pass through the original state
                if (nextPromise) {
                    if (promise->state == PromiseState::FULFILLED) {
                        nova_promise_fulfill(nextPromise, promise->value);
                    } else {
                        nova_promise_reject_internal(nextPromise, promise->error);
                    }
                }
                break;
        }
    }

    promise->callbacks.clear();
}

// Resolve a promise (fulfill it)
void nova_promise_fulfill(void* promisePtr, int64_t value) {
    if (!promisePtr) return;
    NovaPromise* promise = static_cast<NovaPromise*>(promisePtr);

    {
        std::lock_guard<std::mutex> lock(promise->mutex);
        if (promise->state != PromiseState::PENDING) {
            return; // Already settled
        }

        promise->state = PromiseState::FULFILLED;
        promise->value = value;
        promise->hasValue = true;
    }

    promise->cv.notify_all();

    // Process callbacks asynchronously (microtask)
    nova_promise_queue_microtask([promise]() {
        nova_promise_process_callbacks(promise);
    });
    nova_promise_process_microtasks();
}

// Reject a promise (internal)
void nova_promise_reject_internal(void* promisePtr, int64_t reason) {
    if (!promisePtr) return;
    NovaPromise* promise = static_cast<NovaPromise*>(promisePtr);

    {
        std::lock_guard<std::mutex> lock(promise->mutex);
        if (promise->state != PromiseState::PENDING) {
            return; // Already settled
        }

        promise->state = PromiseState::REJECTED;
        promise->error = reason;
        promise->hasError = true;
    }

    promise->cv.notify_all();

    // Process callbacks
    nova_promise_queue_microtask([promise]() {
        nova_promise_process_callbacks(promise);
    });
    nova_promise_process_microtasks();
}

// External reject function
void nova_promise_reject_value(void* promisePtr, int64_t reason) {
    nova_promise_reject_internal(promisePtr, reason);
}

// ============================================================================
// Promise Methods
// ============================================================================

// promise.then(onFulfilled) - returns new Promise
void* nova_promise_then(void* promisePtr, void* onFulfilled) {
    if (!promisePtr) return nova_promise_reject(-1);
    NovaPromise* promise = static_cast<NovaPromise*>(promisePtr);

    // Create new promise for chaining
    NovaPromise* nextPromise = static_cast<NovaPromise*>(nova_promise_create());

    {
        std::lock_guard<std::mutex> lock(promise->mutex);

        if (promise->state == PromiseState::PENDING) {
            // Add callback for later
            PromiseCallback cb;
            cb.type = PromiseCallback::Type::THEN;
            cb.callback = onFulfilled;
            cb.nextPromise = nextPromise;
            promise->callbacks.push_back(cb);
        } else if (promise->state == PromiseState::FULFILLED) {
            // Already fulfilled, schedule callback
            int64_t value = promise->value;
            nova_promise_queue_microtask([onFulfilled, value, nextPromise]() {
                if (onFulfilled) {
                    typedef int64_t (*ThenCallback)(int64_t);
                    try {
                        int64_t result = reinterpret_cast<ThenCallback>(onFulfilled)(value);
                        nova_promise_fulfill(nextPromise, result);
                    } catch (...) {
                        nova_promise_reject_internal(nextPromise, -1);
                    }
                } else {
                    nova_promise_fulfill(nextPromise, value);
                }
            });
            nova_promise_process_microtasks();
        } else {
            // Rejected, pass through
            nova_promise_reject_internal(nextPromise, promise->error);
        }
    }

    return nextPromise;
}

// promise.catch(onRejected) - returns new Promise
void* nova_promise_catch(void* promisePtr, void* onRejected) {
    if (!promisePtr) return nova_promise_reject(-1);
    NovaPromise* promise = static_cast<NovaPromise*>(promisePtr);

    NovaPromise* nextPromise = static_cast<NovaPromise*>(nova_promise_create());

    {
        std::lock_guard<std::mutex> lock(promise->mutex);

        if (promise->state == PromiseState::PENDING) {
            PromiseCallback cb;
            cb.type = PromiseCallback::Type::CATCH;
            cb.callback = onRejected;
            cb.nextPromise = nextPromise;
            promise->callbacks.push_back(cb);
        } else if (promise->state == PromiseState::REJECTED) {
            int64_t error = promise->error;
            nova_promise_queue_microtask([onRejected, error, nextPromise]() {
                if (onRejected) {
                    typedef int64_t (*CatchCallback)(int64_t);
                    try {
                        int64_t result = reinterpret_cast<CatchCallback>(onRejected)(error);
                        nova_promise_fulfill(nextPromise, result);
                    } catch (...) {
                        nova_promise_reject_internal(nextPromise, -1);
                    }
                } else {
                    nova_promise_reject_internal(nextPromise, error);
                }
            });
            nova_promise_process_microtasks();
        } else {
            // Fulfilled, pass through
            nova_promise_fulfill(nextPromise, promise->value);
        }
    }

    return nextPromise;
}

// promise.finally(onFinally) - returns new Promise
void* nova_promise_finally(void* promisePtr, void* onFinally) {
    if (!promisePtr) return nova_promise_reject(-1);
    NovaPromise* promise = static_cast<NovaPromise*>(promisePtr);

    NovaPromise* nextPromise = static_cast<NovaPromise*>(nova_promise_create());

    {
        std::lock_guard<std::mutex> lock(promise->mutex);

        if (promise->state == PromiseState::PENDING) {
            PromiseCallback cb;
            cb.type = PromiseCallback::Type::FINALLY;
            cb.callback = onFinally;
            cb.nextPromise = nextPromise;
            promise->callbacks.push_back(cb);
        } else {
            int64_t value = promise->value;
            int64_t error = promise->error;
            PromiseState state = promise->state;

            nova_promise_queue_microtask([onFinally, value, error, state, nextPromise]() {
                if (onFinally) {
                    typedef void (*FinallyCallback)();
                    reinterpret_cast<FinallyCallback>(onFinally)();
                }
                if (state == PromiseState::FULFILLED) {
                    nova_promise_fulfill(nextPromise, value);
                } else {
                    nova_promise_reject_internal(nextPromise, error);
                }
            });
            nova_promise_process_microtasks();
        }
    }

    return nextPromise;
}

// ============================================================================
// Promise Static Methods
// ============================================================================

// Helper struct to extract Nova array metadata
struct PromiseArrayMeta { char pad[24]; int64_t length; int64_t capacity; int64_t* elements; };

// Promise.all(promises) - Wait for all promises
// Accepts a NovaArray pointer (single argument from compiler)
void* nova_promise_all(void* arrayPtr) {
    if (!arrayPtr) return nova_promise_resolve(0);
    PromiseArrayMeta* meta = static_cast<PromiseArrayMeta*>(arrayPtr);
    int64_t count = meta->length;
    if (count == 0) {
        return nova_promise_resolve(0);
    }

    NovaPromise* result = static_cast<NovaPromise*>(nova_promise_create());

    for (int64_t i = 0; i < count; i++) {
        NovaPromise* p = reinterpret_cast<NovaPromise*>(meta->elements[i]);
        if (p && p->state == PromiseState::REJECTED) {
            nova_promise_reject_internal(result, p->error);
            return result;
        }
    }

    // All fulfilled (or still pending - simplified)
    nova_promise_fulfill(result, count);
    return result;
}

// Promise.race(promises) - First promise to settle wins
void* nova_promise_race(void* arrayPtr) {
    if (!arrayPtr) return nova_promise_create();
    PromiseArrayMeta* meta = static_cast<PromiseArrayMeta*>(arrayPtr);
    int64_t count = meta->length;
    if (count == 0) {
        return nova_promise_create(); // Never settles
    }

    NovaPromise* result = static_cast<NovaPromise*>(nova_promise_create());

    for (int64_t i = 0; i < count; i++) {
        NovaPromise* p = reinterpret_cast<NovaPromise*>(meta->elements[i]);
        if (p) {
            if (p->state == PromiseState::FULFILLED) {
                nova_promise_fulfill(result, p->value);
                return result;
            } else if (p->state == PromiseState::REJECTED) {
                nova_promise_reject_internal(result, p->error);
                return result;
            }
        }
    }

    return result;
}

// Promise.allSettled(promises) - Wait for all to settle (ES2020)
void* nova_promise_allSettled(void* arrayPtr) {
    if (!arrayPtr) return nova_promise_resolve(0);
    PromiseArrayMeta* meta = static_cast<PromiseArrayMeta*>(arrayPtr);
    int64_t count = meta->length;
    return nova_promise_resolve(count);
}

// Promise.any(promises) - First fulfilled promise wins (ES2021)
void* nova_promise_any(void* arrayPtr) {
    if (!arrayPtr) return nova_promise_reject(-1);
    PromiseArrayMeta* meta = static_cast<PromiseArrayMeta*>(arrayPtr);
    int64_t count = meta->length;
    if (count == 0) {
        return nova_promise_reject(-1); // AggregateError
    }

    NovaPromise* result = static_cast<NovaPromise*>(nova_promise_create());

    for (int64_t i = 0; i < count; i++) {
        NovaPromise* p = reinterpret_cast<NovaPromise*>(meta->elements[i]);
        if (p && p->state == PromiseState::FULFILLED) {
            nova_promise_fulfill(result, p->value);
            return result;
        }
    }

    // All rejected
    nova_promise_reject_internal(result, -1);
    return result;
}

// Promise.withResolvers() - Returns { promise, resolve, reject } (ES2024)
// For simplicity, returns the promise pointer. resolve/reject are handled separately.
struct PromiseWithResolvers {
    void* promise;
    void* resolve;  // Function pointer placeholder
    void* reject;   // Function pointer placeholder
};

void* nova_promise_withResolvers() {
    PromiseWithResolvers* result = new PromiseWithResolvers();
    result->promise = nova_promise_create();
    result->resolve = nullptr;  // Would be function pointers in full impl
    result->reject = nullptr;
    return result;
}

// Get promise from withResolvers result
void* nova_promise_withResolvers_promise(void* resolversPtr) {
    if (!resolversPtr) return nullptr;
    PromiseWithResolvers* resolvers = static_cast<PromiseWithResolvers*>(resolversPtr);
    return resolvers->promise;
}

// Resolve the promise from withResolvers
void nova_promise_withResolvers_resolve(void* resolversPtr, int64_t value) {
    if (!resolversPtr) return;
    PromiseWithResolvers* resolvers = static_cast<PromiseWithResolvers*>(resolversPtr);
    nova_promise_fulfill(resolvers->promise, value);
}

// Reject the promise from withResolvers
void nova_promise_withResolvers_reject(void* resolversPtr, int64_t reason) {
    if (!resolversPtr) return;
    PromiseWithResolvers* resolvers = static_cast<PromiseWithResolvers*>(resolversPtr);
    nova_promise_reject_internal(resolvers->promise, reason);
}

// ============================================================================
// Await Support
// ============================================================================

// await promise - blocks until promise settles (simplified synchronous wait)
int64_t nova_promise_await(void* promisePtr) {
    if (!promisePtr) return 0;
    NovaPromise* promise = static_cast<NovaPromise*>(promisePtr);

    // Process any pending microtasks first
    nova_promise_process_microtasks();

    // Wait for promise to settle
    {
        std::unique_lock<std::mutex> lock(promise->mutex);
        promise->cv.wait(lock, [promise]() {
            return promise->state != PromiseState::PENDING;
        });
    }

    if (promise->state == PromiseState::FULFILLED) {
        return promise->value;
    } else {
        // In a real implementation, this would throw
        return promise->error;
    }
}

// Check if promise is fulfilled
int64_t nova_promise_is_fulfilled(void* promisePtr) {
    if (!promisePtr) return 0;
    NovaPromise* promise = static_cast<NovaPromise*>(promisePtr);
    return promise->state == PromiseState::FULFILLED ? 1 : 0;
}

// Check if promise is rejected
int64_t nova_promise_is_rejected(void* promisePtr) {
    if (!promisePtr) return 0;
    NovaPromise* promise = static_cast<NovaPromise*>(promisePtr);
    return promise->state == PromiseState::REJECTED ? 1 : 0;
}

// Check if promise is pending
int64_t nova_promise_is_pending(void* promisePtr) {
    if (!promisePtr) return 0;
    NovaPromise* promise = static_cast<NovaPromise*>(promisePtr);
    return promise->state == PromiseState::PENDING ? 1 : 0;
}

// Get promise value (only valid if fulfilled)
int64_t nova_promise_get_value(void* promisePtr) {
    if (!promisePtr) return 0;
    NovaPromise* promise = static_cast<NovaPromise*>(promisePtr);
    return promise->value;
}

// Get promise error (only valid if rejected)
int64_t nova_promise_get_error(void* promisePtr) {
    if (!promisePtr) return 0;
    NovaPromise* promise = static_cast<NovaPromise*>(promisePtr);
    return promise->error;
}

// promise.then(onFulfilled, onRejected) - full version with both callbacks
void* nova_promise_then_both(void* promisePtr, void* onFulfilled, void* onRejected) {
    if (!promisePtr) return nova_promise_reject(-1);
    NovaPromise* promise = static_cast<NovaPromise*>(promisePtr);

    NovaPromise* nextPromise = static_cast<NovaPromise*>(nova_promise_create());

    {
        std::lock_guard<std::mutex> lock(promise->mutex);

        if (promise->state == PromiseState::PENDING) {
            PromiseCallback thenCb;
            thenCb.type = PromiseCallback::Type::THEN;
            thenCb.callback = onFulfilled;
            thenCb.nextPromise = nextPromise;
            promise->callbacks.push_back(thenCb);

            if (onRejected) {
                PromiseCallback catchCb;
                catchCb.type = PromiseCallback::Type::CATCH;
                catchCb.callback = onRejected;
                catchCb.nextPromise = nextPromise;
                promise->callbacks.push_back(catchCb);
            }
        } else if (promise->state == PromiseState::FULFILLED) {
            int64_t value = promise->value;
            nova_promise_queue_microtask([onFulfilled, value, nextPromise]() {
                if (onFulfilled) {
                    typedef int64_t (*ThenCallback)(int64_t);
                    try {
                        int64_t result = reinterpret_cast<ThenCallback>(onFulfilled)(value);
                        nova_promise_fulfill(nextPromise, result);
                    } catch (...) {
                        nova_promise_reject_internal(nextPromise, -1);
                    }
                } else {
                    nova_promise_fulfill(nextPromise, value);
                }
            });
            nova_promise_process_microtasks();
        } else {
            int64_t error = promise->error;
            if (onRejected) {
                nova_promise_queue_microtask([onRejected, error, nextPromise]() {
                    typedef int64_t (*CatchCallback)(int64_t);
                    try {
                        int64_t result = reinterpret_cast<CatchCallback>(onRejected)(error);
                        nova_promise_fulfill(nextPromise, result);
                    } catch (...) {
                        nova_promise_reject_internal(nextPromise, -1);
                    }
                });
                nova_promise_process_microtasks();
            } else {
                nova_promise_reject_internal(nextPromise, error);
            }
        }
    }

    return nextPromise;
}

// Promise.try(fn) - ES2025: Wraps function in try/catch and returns promise
void* nova_promise_try(void* fn) {
    NovaPromise* promise = static_cast<NovaPromise*>(nova_promise_create());

    if (!fn) {
        nova_promise_fulfill(promise, 0);
        return promise;
    }

    typedef int64_t (*TryCallback)();
    try {
        int64_t result = reinterpret_cast<TryCallback>(fn)();
        nova_promise_fulfill(promise, result);
    } catch (...) {
        nova_promise_reject_internal(promise, -1);
    }

    return promise;
}

// Promise.try with args - ES2025
void* nova_promise_try_with_args(void* fn, int64_t* args, int argCount) {
    NovaPromise* promise = static_cast<NovaPromise*>(nova_promise_create());

    if (!fn) {
        nova_promise_fulfill(promise, 0);
        return promise;
    }

    try {
        int64_t result = 0;
        switch (argCount) {
            case 0: {
                typedef int64_t (*Fn0)();
                result = reinterpret_cast<Fn0>(fn)();
                break;
            }
            case 1: {
                typedef int64_t (*Fn1)(int64_t);
                result = reinterpret_cast<Fn1>(fn)(args[0]);
                break;
            }
            case 2: {
                typedef int64_t (*Fn2)(int64_t, int64_t);
                result = reinterpret_cast<Fn2>(fn)(args[0], args[1]);
                break;
            }
            case 3:
            default: {
                typedef int64_t (*Fn3)(int64_t, int64_t, int64_t);
                result = reinterpret_cast<Fn3>(fn)(args[0], args[1], args[2]);
                break;
            }
        }
        nova_promise_fulfill(promise, result);
    } catch (...) {
        nova_promise_reject_internal(promise, -1);
    }

    return promise;
}

// Free a promise (cleanup)
void nova_promise_free(void* promisePtr) {
    if (!promisePtr) return;
    NovaPromise* promise = static_cast<NovaPromise*>(promisePtr);
    delete promise;
}

// Free withResolvers result
void nova_promise_withResolvers_free(void* resolversPtr) {
    if (!resolversPtr) return;
    PromiseWithResolvers* resolvers = static_cast<PromiseWithResolvers*>(resolversPtr);
    delete resolvers;
}

// Get promise state as string
const char* nova_promise_get_state(void* promisePtr) {
    if (!promisePtr) return "unknown";
    NovaPromise* promise = static_cast<NovaPromise*>(promisePtr);
    switch (promise->state) {
        case PromiseState::PENDING: return "pending";
        case PromiseState::FULFILLED: return "fulfilled";
        case PromiseState::REJECTED: return "rejected";
        default: return "unknown";
    }
}

// Symbol.toStringTag support - returns "[object Promise]"
const char* nova_promise_toString(void* promisePtr) {
    (void)promisePtr;
    return "[object Promise]";
}

// Check if value is a Promise
int64_t nova_promise_isPromise(void* value) {
    if (!value) return 0;
    NovaPromise* p = static_cast<NovaPromise*>(value);
    return (p->state == PromiseState::PENDING ||
            p->state == PromiseState::FULFILLED ||
            p->state == PromiseState::REJECTED) ? 1 : 0;
}

// Run microtask checkpoint
void nova_promise_runMicrotasks() {
    nova_promise_process_microtasks();
}

// Check if microtask queue is empty
int64_t nova_promise_hasPendingMicrotasks() {
    std::lock_guard<std::mutex> lock(microtaskMutex);
    return microtaskQueue.empty() ? 0 : 1;
}

// queueMicrotask - internal Promise API version (main one in Timers.cpp)
void nova_promise_queueMicrotaskInternal(void* callback) {
    if (!callback) return;
    typedef void (*MicrotaskCallback)();
    MicrotaskCallback cb = reinterpret_cast<MicrotaskCallback>(callback);
    nova_promise_queue_microtask([cb]() {
        cb();
    });
}

} // extern "C"
