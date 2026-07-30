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
#include <unordered_set>
#include <memory>

#include "nova/runtime/Value.h"
#include "nova/runtime/Runtime.h"

extern "C" {

// Forward declarations
void nova_console_log_string(const char* str);
void nova_console_error_string(const char* str);
void* nova_dynamic_object_create();
void nova_dynamic_object_set_tagged(
    void* object, const char* key, std::uint64_t value);
void* nova_dynamic_object_get_registered_function(
    std::uint64_t value, const char* key);
void* nova_dynamic_object_from_registered_value(std::uint64_t value);
int64_t nova_dynamic_call_method_2(
    void* object, const char* methodName, int64_t arg0, int64_t arg1);

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
    void* callback;      // Fulfillment/catch/finally function pointer
    void* rejectedCallback; // Optional rejection function for then(resolve, reject)
    void* nextPromise;   // Promise to chain result to
    void* callbackEnvironment = nullptr;
    void* rejectedEnvironment = nullptr;
    bool callbackAcceptsValue = true;
    bool rejectedAcceptsValue = true;
};

// ============================================================================
// Promise Structure
// ============================================================================
struct NovaPromise {
    PromiseState state;
    int64_t value;           // Fulfilled value (for simplicity, using int64_t)
    int64_t error;           // Rejection reason
    std::vector<PromiseCallback> callbacks;
    std::vector<std::function<void(PromiseState, int64_t)>> observers;
    std::mutex mutex;
    std::condition_variable cv;
    bool hasValue;
    bool hasError;
    void* resolveCallable;
    void* rejectCallable;
};

enum class PromiseCallableKind {
    Resolve,
    Reject
};

struct PromiseCallable {
    PromiseCallableKind kind;
    NovaPromise* promise;
};

static std::unordered_set<PromiseCallable*> callableRegistry;
static std::mutex callableRegistryMutex;

static PromiseCallable* callableFromValue(int64_t value) {
    auto* candidate = static_cast<PromiseCallable*>(
        nova_value_to_object(static_cast<std::uint64_t>(value)));
    if (!candidate) return nullptr;
    std::lock_guard<std::mutex> lock(callableRegistryMutex);
    return callableRegistry.count(candidate) != 0 ? candidate : nullptr;
}

static PromiseCallable* createPromiseCallable(
        NovaPromise* promise, PromiseCallableKind kind) {
    auto* callable = new PromiseCallable{kind, promise};
    std::lock_guard<std::mutex> lock(callableRegistryMutex);
    callableRegistry.insert(callable);
    return callable;
}

static thread_local NovaPromise* currentExecutorPromise = nullptr;
static std::unordered_set<NovaPromise*> promiseRegistry;
static std::mutex promiseRegistryMutex;

static void registerPromise(NovaPromise* promise) {
    std::lock_guard<std::mutex> lock(promiseRegistryMutex);
    promiseRegistry.insert(promise);
}

static bool isRegisteredPromise(NovaPromise* promise) {
    std::lock_guard<std::mutex> lock(promiseRegistryMutex);
    return promiseRegistry.count(promise) != 0;
}

static NovaPromise* promiseFromValue(int64_t value) {
    using namespace nova::runtime;
    const auto bits = static_cast<JSValue>(value);
    NovaPromise* candidate = nullptr;
    if (js_value_has_tag(bits, JS_VALUE_OBJECT_TAG)) {
        candidate = reinterpret_cast<NovaPromise*>(static_cast<std::uintptr_t>(
            bits & JS_VALUE_PAYLOAD_MASK));
    } else {
        candidate = reinterpret_cast<NovaPromise*>(static_cast<std::uintptr_t>(bits));
    }
    return candidate && isRegisteredPromise(candidate) ? candidate : nullptr;
}

// ============================================================================
// Microtask Queue (for proper Promise scheduling)
// ============================================================================
static std::queue<std::function<void()>> microtaskQueue;
static std::mutex microtaskMutex;
static std::atomic<bool> processingMicrotasks{false};

static int64_t invokePromiseCallback(
        void* callback, int64_t value, void* environment, bool acceptsValue) {
    if (!callback) {
        return static_cast<int64_t>(nova::runtime::JS_VALUE_UNDEFINED);
    }
    if (environment) {
        if (acceptsValue) {
            using Callback = int64_t (*)(int64_t, void*);
            return reinterpret_cast<Callback>(callback)(value, environment);
        }
        using Callback = int64_t (*)(void*);
        return reinterpret_cast<Callback>(callback)(environment);
    }
    if (acceptsValue) {
        using Callback = int64_t (*)(int64_t);
        return reinterpret_cast<Callback>(callback)(value);
    }
    using Callback = int64_t (*)();
    return reinterpret_cast<Callback>(callback)();
}

static void invokeFinallyCallback(void* callback, void* environment) {
    if (!callback) return;
    if (environment) {
        using Callback = void (*)(void*);
        reinterpret_cast<Callback>(callback)(environment);
        return;
    }
    using Callback = void (*)();
    reinterpret_cast<Callback>(callback)();
}

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
    promise->resolveCallable = nullptr;
    promise->rejectCallable = nullptr;
    registerPromise(promise);
    return promise;
}

void nova_promise_executor_resolve(int64_t value) {
    if (currentExecutorPromise) {
        nova_promise_fulfill(currentExecutorPromise, value);
    }
}

void nova_promise_executor_reject(int64_t reason) {
    if (currentExecutorPromise) {
        nova_promise_reject_internal(currentExecutorPromise, reason);
    }
}

int64_t nova_callable_call1(int64_t callableValue, int64_t argument) {
    PromiseCallable* callable = callableFromValue(callableValue);
    if (!callable || !callable->promise) {
        return static_cast<int64_t>(nova::runtime::JS_VALUE_UNDEFINED);
    }
    if (callable->kind == PromiseCallableKind::Resolve) {
        nova_promise_fulfill(callable->promise, argument);
    } else {
        nova_promise_reject_internal(callable->promise, argument);
    }
    return static_cast<int64_t>(nova::runtime::JS_VALUE_UNDEFINED);
}

void* nova_promise_construct(void* executor, void* environment) {
    NovaPromise* promise = static_cast<NovaPromise*>(nova_promise_create());
    if (!executor) {
        nova_promise_reject_internal(
            promise, static_cast<int64_t>(0x7ff9000000000000ULL));
        return promise;
    }

    PromiseCallable* resolve = createPromiseCallable(
        promise, PromiseCallableKind::Resolve);
    PromiseCallable* reject = createPromiseCallable(
        promise, PromiseCallableKind::Reject);
    promise->resolveCallable = resolve;
    promise->rejectCallable = reject;

    using Executor = void (*)(int64_t, int64_t, void*);
    NovaPromise* previousExecutorPromise = currentExecutorPromise;
    currentExecutorPromise = promise;
    try {
        reinterpret_cast<Executor>(executor)(
            static_cast<int64_t>(nova_value_from_object(resolve)),
            static_cast<int64_t>(nova_value_from_object(reject)),
            environment);
    } catch (...) {
        nova_promise_reject_internal(promise, -1);
    }
    currentExecutorPromise = previousExecutorPromise;
    return promise;
}

// Promise.resolve(value) - Create an already-fulfilled promise
void* nova_promise_resolve(int64_t value) {
    if (NovaPromise* existing = promiseFromValue(value)) {
        return existing;
    }
    NovaPromise* promise = static_cast<NovaPromise*>(nova_promise_create());
    nova_promise_fulfill(promise, value);
    return promise;
}

// Promise.reject(reason) - Create an already-rejected promise
void* nova_promise_reject(int64_t reason) {
    NovaPromise* promise = static_cast<NovaPromise*>(nova_promise_create());
    nova_promise_reject_internal(promise, reason);
    return promise;
}

// ============================================================================
// Promise Resolution
// ============================================================================

// Internal: Process callbacks when promise settles
void nova_promise_process_callbacks(NovaPromise* promise) {
    std::vector<PromiseCallback> callbacks;
    std::vector<std::function<void(PromiseState, int64_t)>> observers;
    PromiseState state;
    int64_t value;
    int64_t error;
    {
        std::lock_guard<std::mutex> lock(promise->mutex);
        callbacks.swap(promise->callbacks);
        observers.swap(promise->observers);
        state = promise->state;
        value = promise->value;
        error = promise->error;
    }

    for (auto& observer : observers) {
        observer(state, state == PromiseState::FULFILLED ? value : error);
    }

    for (auto& cb : callbacks) {
        NovaPromise* nextPromise = static_cast<NovaPromise*>(cb.nextPromise);

        switch (cb.type) {
            case PromiseCallback::Type::THEN:
                if (state == PromiseState::FULFILLED && cb.callback) {
                    try {
                        int64_t result = invokePromiseCallback(
                            cb.callback, value, cb.callbackEnvironment,
                            cb.callbackAcceptsValue);
                        if (nextPromise) {
                            nova_promise_fulfill(nextPromise, result);
                        }
                    } catch (...) {
                        if (nextPromise) {
                            nova_promise_reject_internal(nextPromise, -1);
                        }
                    }
                } else if (state == PromiseState::REJECTED &&
                           cb.rejectedCallback) {
                    try {
                        int64_t result = invokePromiseCallback(
                            cb.rejectedCallback, error,
                            cb.rejectedEnvironment,
                            cb.rejectedAcceptsValue);
                        if (nextPromise) {
                            nova_promise_fulfill(nextPromise, result);
                        }
                    } catch (...) {
                        if (nextPromise) {
                            nova_promise_reject_internal(nextPromise, -1);
                        }
                    }
                } else if (nextPromise) {
                    if (state == PromiseState::FULFILLED) {
                        nova_promise_fulfill(nextPromise, value);
                    } else {
                        nova_promise_reject_internal(nextPromise, error);
                    }
                }
                break;

            case PromiseCallback::Type::CATCH:
                if (state == PromiseState::REJECTED && cb.callback) {
                    try {
                        int64_t result = invokePromiseCallback(
                            cb.callback, error, cb.callbackEnvironment,
                            cb.callbackAcceptsValue);
                        if (nextPromise) {
                            nova_promise_fulfill(nextPromise, result);
                        }
                    } catch (...) {
                        if (nextPromise) {
                            nova_promise_reject_internal(nextPromise, -1);
                        }
                    }
                } else if (state == PromiseState::FULFILLED && nextPromise) {
                    // Pass fulfillment to next promise
                    nova_promise_fulfill(nextPromise, value);
                } else if (state == PromiseState::REJECTED && nextPromise) {
                    nova_promise_reject_internal(nextPromise, error);
                }
                break;

            case PromiseCallback::Type::FINALLY:
                if (cb.callback) {
                    invokeFinallyCallback(
                        cb.callback, cb.callbackEnvironment);
                }
                // Pass through the original state
                if (nextPromise) {
                    if (state == PromiseState::FULFILLED) {
                        nova_promise_fulfill(nextPromise, value);
                    } else {
                        nova_promise_reject_internal(nextPromise, error);
                    }
                }
                break;
        }
    }
}

static void observePromise(
        NovaPromise* promise,
        std::function<void(PromiseState, int64_t)> observer) {
    PromiseState state;
    int64_t payload;
    {
        std::lock_guard<std::mutex> lock(promise->mutex);
        if (promise->state == PromiseState::PENDING) {
            promise->observers.push_back(std::move(observer));
            return;
        }
        state = promise->state;
        payload = state == PromiseState::FULFILLED
            ? promise->value : promise->error;
    }
    nova_promise_queue_microtask(
        [observer = std::move(observer), state, payload]() mutable {
            observer(state, payload);
        });
}

static void fulfillPlain(NovaPromise* promise, int64_t value) {
    {
        std::lock_guard<std::mutex> lock(promise->mutex);
        if (promise->state != PromiseState::PENDING) {
            return;
        }
        promise->state = PromiseState::FULFILLED;
        promise->value = value;
        promise->hasValue = true;
    }
    promise->cv.notify_all();
    nova_promise_queue_microtask([promise]() {
        nova_promise_process_callbacks(promise);
    });
}

// Apply the Promise Resolution Procedure. Nova Promise objects are adopted;
// ordinary tagged values are fulfilled as-is.
void nova_promise_fulfill(void* promisePtr, int64_t value) {
    if (!promisePtr) return;
    NovaPromise* promise = static_cast<NovaPromise*>(promisePtr);

    NovaPromise* adopted = promiseFromValue(value);
    if (!adopted) {
        if (nova_dynamic_object_get_registered_function(
                static_cast<std::uint64_t>(value), "then")) {
            void* thenable = nova_dynamic_object_from_registered_value(
                static_cast<std::uint64_t>(value));
            PromiseCallable* resolve = createPromiseCallable(
                promise, PromiseCallableKind::Resolve);
            PromiseCallable* reject = createPromiseCallable(
                promise, PromiseCallableKind::Reject);
            nova_dynamic_call_method_2(
                thenable, "then",
                static_cast<int64_t>(nova_value_from_object(resolve)),
                static_cast<int64_t>(nova_value_from_object(reject)));
            return;
        }
        fulfillPlain(promise, value);
        return;
    }
    if (adopted == promise) {
        nova_promise_reject_internal(promise, -1);
        return;
    }

    PromiseState adoptedState;
    int64_t adoptedValue = 0;
    int64_t adoptedError = 0;
    {
        std::lock_guard<std::mutex> lock(adopted->mutex);
        adoptedState = adopted->state;
        adoptedValue = adopted->value;
        adoptedError = adopted->error;
        if (adoptedState == PromiseState::PENDING) {
            adopted->callbacks.push_back({PromiseCallback::Type::THEN,
                nullptr, nullptr, promise});
            return;
        }
    }
    if (adoptedState == PromiseState::FULFILLED) {
        fulfillPlain(promise, adoptedValue);
    } else {
        nova_promise_reject_internal(promise, adoptedError);
    }
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
}

// External reject function
void nova_promise_reject_value(void* promisePtr, int64_t reason) {
    nova_promise_reject_internal(promisePtr, reason);
}

// ============================================================================
// Promise Methods
// ============================================================================

// promise.then(onFulfilled) - returns new Promise
void* nova_promise_then(
        void* promisePtr, void* onFulfilled, void* environment,
        int64_t acceptsValue) {
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
            cb.rejectedCallback = nullptr;
            cb.nextPromise = nextPromise;
            cb.callbackEnvironment = environment;
            cb.callbackAcceptsValue = acceptsValue != 0;
            promise->callbacks.push_back(cb);
        } else if (promise->state == PromiseState::FULFILLED) {
            // Already fulfilled, schedule callback
            int64_t value = promise->value;
            nova_promise_queue_microtask(
                [onFulfilled, environment, acceptsValue, value, nextPromise]() {
                if (onFulfilled) {
                    try {
                        int64_t result = invokePromiseCallback(
                            onFulfilled, value, environment,
                            acceptsValue != 0);
                        nova_promise_fulfill(nextPromise, result);
                    } catch (...) {
                        nova_promise_reject_internal(nextPromise, -1);
                    }
                } else {
                    nova_promise_fulfill(nextPromise, value);
                }
            });
        } else {
            // Rejected, pass through
            nova_promise_reject_internal(nextPromise, promise->error);
        }
    }

    return nextPromise;
}

// promise.catch(onRejected) - returns new Promise
void* nova_promise_catch(
        void* promisePtr, void* onRejected, void* environment,
        int64_t acceptsValue) {
    if (!promisePtr) return nova_promise_reject(-1);
    NovaPromise* promise = static_cast<NovaPromise*>(promisePtr);

    NovaPromise* nextPromise = static_cast<NovaPromise*>(nova_promise_create());

    {
        std::lock_guard<std::mutex> lock(promise->mutex);

        if (promise->state == PromiseState::PENDING) {
            PromiseCallback cb;
            cb.type = PromiseCallback::Type::CATCH;
            cb.callback = onRejected;
            cb.rejectedCallback = nullptr;
            cb.nextPromise = nextPromise;
            cb.callbackEnvironment = environment;
            cb.callbackAcceptsValue = acceptsValue != 0;
            promise->callbacks.push_back(cb);
        } else if (promise->state == PromiseState::REJECTED) {
            int64_t error = promise->error;
            nova_promise_queue_microtask(
                [onRejected, environment, acceptsValue, error, nextPromise]() {
                if (onRejected) {
                    try {
                        int64_t result = invokePromiseCallback(
                            onRejected, error, environment,
                            acceptsValue != 0);
                        nova_promise_fulfill(nextPromise, result);
                    } catch (...) {
                        nova_promise_reject_internal(nextPromise, -1);
                    }
                } else {
                    nova_promise_reject_internal(nextPromise, error);
                }
            });
        } else {
            // Fulfilled, pass through
            nova_promise_fulfill(nextPromise, promise->value);
        }
    }

    return nextPromise;
}

// promise.finally(onFinally) - returns new Promise
void* nova_promise_finally(
        void* promisePtr, void* onFinally, void* environment,
        [[maybe_unused]] int64_t acceptsValue) {
    if (!promisePtr) return nova_promise_reject(-1);
    NovaPromise* promise = static_cast<NovaPromise*>(promisePtr);

    NovaPromise* nextPromise = static_cast<NovaPromise*>(nova_promise_create());

    {
        std::lock_guard<std::mutex> lock(promise->mutex);

        if (promise->state == PromiseState::PENDING) {
            PromiseCallback cb;
            cb.type = PromiseCallback::Type::FINALLY;
            cb.callback = onFinally;
            cb.rejectedCallback = nullptr;
            cb.nextPromise = nextPromise;
            cb.callbackEnvironment = environment;
            cb.callbackAcceptsValue = false;
            promise->callbacks.push_back(cb);
        } else {
            int64_t value = promise->value;
            int64_t error = promise->error;
            PromiseState state = promise->state;

            nova_promise_queue_microtask(
                [onFinally, environment, value, error, state, nextPromise]() {
                if (onFinally) {
                    invokeFinallyCallback(onFinally, environment);
                }
                if (state == PromiseState::FULFILLED) {
                    nova_promise_fulfill(nextPromise, value);
                } else {
                    nova_promise_reject_internal(nextPromise, error);
                }
            });
        }
    }

    return nextPromise;
}

// ============================================================================
// Promise Static Methods
// ============================================================================

// Helper struct to extract Nova array metadata
struct PromiseArrayMeta { char pad[24]; int64_t length; int64_t capacity; int64_t* elements; };

void* nova_value_array_create(int64_t length);
void value_array_set(void* arrayPtr, int64_t index, int64_t value);

static int64_t normalizePlainPromiseElement(int64_t value) {
    using namespace nova::runtime;
    const auto bits = static_cast<JSValue>(value);
    const auto tag = bits & JS_VALUE_TAG_MASK;
    if (tag == JS_VALUE_UNDEFINED || tag == JS_VALUE_NULL ||
        tag == JS_VALUE_FALSE || tag == JS_VALUE_TRUE ||
        tag == JS_VALUE_STRING_TAG || tag == JS_VALUE_OBJECT_TAG ||
        tag == JS_VALUE_CANONICAL_NAN) {
        return value;
    }
    if (value >= -(1LL << 31) && value <= (1LL << 31)) {
        return static_cast<int64_t>(nova_value_from_i64(value));
    }
    return value;
}

// Promise.all(promises) - Wait for all promises
// Accepts a NovaArray pointer (single argument from compiler)
void* nova_promise_all(void* arrayPtr) {
    if (!arrayPtr) return nova_promise_reject(-1);
    PromiseArrayMeta* meta = static_cast<PromiseArrayMeta*>(arrayPtr);
    int64_t count = meta->length;
    void* values = nova_value_array_create(count);
    // Promise fulfillment payloads are NaN-boxed JSValues. Mark the result
    // array so typed and dynamic element access do not box them a second time.
    static_cast<nova::runtime::ObjectHeader*>(values)->value_encoding = 1;
    if (count == 0) {
        return nova_promise_resolve(static_cast<int64_t>(
            nova_value_from_object(values)));
    }

    NovaPromise* result = static_cast<NovaPromise*>(nova_promise_create());
    struct AllContext {
        std::mutex mutex;
        int64_t remaining;
        bool settled = false;
        NovaPromise* result;
        void* values;
    };
    auto context = std::make_shared<AllContext>();
    context->remaining = count;
    context->result = result;
    context->values = values;

    for (int64_t index = 0; index < count; ++index) {
        const int64_t element = meta->elements[index];
        auto settleElement = [context, index](
                PromiseState state, int64_t payload) {
            std::lock_guard<std::mutex> lock(context->mutex);
            if (context->settled) return;
            if (state == PromiseState::REJECTED) {
                context->settled = true;
                nova_promise_reject_internal(context->result, payload);
                return;
            }
            value_array_set(context->values, index, payload);
            if (--context->remaining == 0) {
                context->settled = true;
                nova_promise_fulfill(context->result, static_cast<int64_t>(
                    nova_value_from_object(context->values)));
            }
        };
        if (NovaPromise* promise = promiseFromValue(element)) {
            observePromise(promise, std::move(settleElement));
        } else {
            settleElement(PromiseState::FULFILLED,
                          normalizePlainPromiseElement(element));
        }
    }
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
    for (int64_t index = 0; index < count; ++index) {
        const int64_t element = meta->elements[index];
        auto settle = [result](PromiseState state, int64_t payload) {
            if (state == PromiseState::FULFILLED) {
                nova_promise_fulfill(result, payload);
            } else {
                nova_promise_reject_internal(result, payload);
            }
        };
        if (NovaPromise* promise = promiseFromValue(element)) {
            observePromise(promise, std::move(settle));
        } else {
            nova_promise_queue_microtask(
                [settle = std::move(settle), element]() mutable {
                    settle(PromiseState::FULFILLED,
                           normalizePlainPromiseElement(element));
                });
        }
    }

    return result;
}

// Promise.allSettled(promises) - Wait for all to settle (ES2020)
void* nova_promise_allSettled(void* arrayPtr) {
    if (!arrayPtr) return nova_promise_resolve(0);
    PromiseArrayMeta* meta = static_cast<PromiseArrayMeta*>(arrayPtr);
    int64_t count = meta->length;
    void* results = nova_value_array_create(count);
    if (count == 0) {
        return nova_promise_resolve(static_cast<int64_t>(
            nova_value_from_object(results)));
    }

    NovaPromise* result =
        static_cast<NovaPromise*>(nova_promise_create());
    struct AllSettledContext {
        std::mutex mutex;
        int64_t remaining;
        NovaPromise* result;
        void* values;
    };
    auto context = std::make_shared<AllSettledContext>();
    context->remaining = count;
    context->result = result;
    context->values = results;

    for (int64_t index = 0; index < count; ++index) {
        const int64_t element = meta->elements[index];
        auto settle = [context, index](
                PromiseState state, int64_t payload) {
            void* entry = nova_dynamic_object_create();
            if (state == PromiseState::FULFILLED) {
                nova_dynamic_object_set_tagged(
                    entry, "status",
                    nova_value_from_string("fulfilled"));
                nova_dynamic_object_set_tagged(
                    entry, "value", static_cast<std::uint64_t>(payload));
            } else {
                nova_dynamic_object_set_tagged(
                    entry, "status",
                    nova_value_from_string("rejected"));
                nova_dynamic_object_set_tagged(
                    entry, "reason", static_cast<std::uint64_t>(payload));
            }

            std::lock_guard<std::mutex> lock(context->mutex);
            value_array_set(context->values, index, static_cast<int64_t>(
                nova_value_from_object(entry)));
            if (--context->remaining == 0) {
                nova_promise_fulfill(
                    context->result, static_cast<int64_t>(
                        nova_value_from_object(context->values)));
            }
        };
        if (NovaPromise* promise = promiseFromValue(element)) {
            observePromise(promise, std::move(settle));
        } else {
            nova_promise_queue_microtask(
                [settle = std::move(settle), element]() mutable {
                    settle(PromiseState::FULFILLED,
                           normalizePlainPromiseElement(element));
                });
        }
    }
    return result;
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
    struct AnyContext {
        std::mutex mutex;
        int64_t remaining;
        bool settled = false;
        NovaPromise* result;
    };
    auto context = std::make_shared<AnyContext>();
    context->remaining = count;
    context->result = result;
    for (int64_t index = 0; index < count; ++index) {
        const int64_t element = meta->elements[index];
        auto settle = [context](PromiseState state, int64_t payload) {
            std::lock_guard<std::mutex> lock(context->mutex);
            if (context->settled) return;
            if (state == PromiseState::FULFILLED) {
                context->settled = true;
                nova_promise_fulfill(context->result, payload);
            } else if (--context->remaining == 0) {
                context->settled = true;
                nova_promise_reject_internal(context->result, -1);
            }
        };
        if (NovaPromise* promise = promiseFromValue(element)) {
            observePromise(promise, std::move(settle));
        } else {
            nova_promise_queue_microtask(
                [settle = std::move(settle), element]() mutable {
                    settle(PromiseState::FULFILLED,
                           normalizePlainPromiseElement(element));
                });
        }
    }
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

// Get resolve function from withResolvers result. Returns a non-null
// placeholder so HIR-level `typeof capability.resolve === "function"`
// holds; the actual callback dispatch goes through Promise.resolve on
// the underlying promise object.
void* nova_promise_withResolvers_resolve_get(void* resolversPtr) {
    if (!resolversPtr) return nullptr;
    PromiseWithResolvers* resolvers = static_cast<PromiseWithResolvers*>(resolversPtr);
    return resolvers->resolve ? resolvers->resolve : resolversPtr;
}

void* nova_promise_withResolvers_reject_get(void* resolversPtr) {
    if (!resolversPtr) return nullptr;
    PromiseWithResolvers* resolvers = static_cast<PromiseWithResolvers*>(resolversPtr);
    return resolvers->reject ? resolvers->reject : resolversPtr;
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
void* nova_promise_then_both(
        void* promisePtr,
        void* onFulfilled, void* fulfilledEnvironment,
        int64_t fulfilledAcceptsValue,
        void* onRejected, void* rejectedEnvironment,
        int64_t rejectedAcceptsValue) {
    if (!promisePtr) return nova_promise_reject(-1);
    NovaPromise* promise = static_cast<NovaPromise*>(promisePtr);

    NovaPromise* nextPromise = static_cast<NovaPromise*>(nova_promise_create());

    {
        std::lock_guard<std::mutex> lock(promise->mutex);

        if (promise->state == PromiseState::PENDING) {
            PromiseCallback thenCb;
            thenCb.type = PromiseCallback::Type::THEN;
            thenCb.callback = onFulfilled;
            thenCb.rejectedCallback = onRejected;
            thenCb.nextPromise = nextPromise;
            thenCb.callbackEnvironment = fulfilledEnvironment;
            thenCb.rejectedEnvironment = rejectedEnvironment;
            thenCb.callbackAcceptsValue = fulfilledAcceptsValue != 0;
            thenCb.rejectedAcceptsValue = rejectedAcceptsValue != 0;
            promise->callbacks.push_back(thenCb);
        } else if (promise->state == PromiseState::FULFILLED) {
            int64_t value = promise->value;
            nova_promise_queue_microtask(
                [onFulfilled, fulfilledEnvironment, fulfilledAcceptsValue,
                 value, nextPromise]() {
                if (onFulfilled) {
                    try {
                        int64_t result = invokePromiseCallback(
                            onFulfilled, value, fulfilledEnvironment,
                            fulfilledAcceptsValue != 0);
                        nova_promise_fulfill(nextPromise, result);
                    } catch (...) {
                        nova_promise_reject_internal(nextPromise, -1);
                    }
                } else {
                    nova_promise_fulfill(nextPromise, value);
                }
            });
        } else {
            int64_t error = promise->error;
            if (onRejected) {
                nova_promise_queue_microtask(
                    [onRejected, rejectedEnvironment, rejectedAcceptsValue,
                     error, nextPromise]() {
                    try {
                        int64_t result = invokePromiseCallback(
                            onRejected, error, rejectedEnvironment,
                            rejectedAcceptsValue != 0);
                        nova_promise_fulfill(nextPromise, result);
                    } catch (...) {
                        nova_promise_reject_internal(nextPromise, -1);
                    }
                });
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
    {
        std::lock_guard<std::mutex> lock(promiseRegistryMutex);
        promiseRegistry.erase(promise);
    }
    {
        std::lock_guard<std::mutex> lock(callableRegistryMutex);
        if (promise->resolveCallable) {
            callableRegistry.erase(
                static_cast<PromiseCallable*>(promise->resolveCallable));
        }
        if (promise->rejectCallable) {
            callableRegistry.erase(
                static_cast<PromiseCallable*>(promise->rejectCallable));
        }
    }
    delete static_cast<PromiseCallable*>(promise->resolveCallable);
    delete static_cast<PromiseCallable*>(promise->rejectCallable);
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
    return value && isRegisteredPromise(static_cast<NovaPromise*>(value)) ? 1 : 0;
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
