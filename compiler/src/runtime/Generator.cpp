// Generator runtime for Nova compiler
// Implements ES6 Generator and ES2018 AsyncGenerator

#include <cstdint>
#include <cstdio>
#include <vector>
#include <string>
#include <functional>
#include <mutex>

// Forward declarations
extern "C" {
    void* nova_generator_create(void* funcPtr, int64_t initialState);
    void* nova_generator_next(void* genPtr, int64_t value);
    void* nova_generator_return(void* genPtr, int64_t value);
    void* nova_generator_throw(void* genPtr, int64_t error);
    int64_t nova_iterator_result_value(void* resultPtr);
    int64_t nova_iterator_result_done(void* resultPtr);
    void nova_generator_yield(void* genPtr, int64_t value);
    void nova_generator_set_state(void* genPtr, int64_t state);
    int64_t nova_generator_get_state(void* genPtr);
    void nova_generator_complete(void* genPtr, int64_t returnValue);

    // AsyncGenerator
    void* nova_async_generator_create(void* funcPtr, int64_t initialState);
    void* nova_async_generator_next(void* genPtr, int64_t value);
    void* nova_async_generator_return(void* genPtr, int64_t value);
    void* nova_async_generator_throw(void* genPtr, int64_t error);
}

// Generator states
enum class GeneratorState {
    CREATED,      // Generator created but not started
    RUNNING,      // Currently executing
    SUSPENDED,    // Paused at yield
    COMPLETED     // Finished (return or throw)
};

// Iterator result { value, done }
struct IteratorResult {
    int64_t value;
    bool done;
};

// Generator object
struct NovaGenerator {
    GeneratorState state;
    void* functionPtr;          // The generator function
    int64_t currentState;       // State machine state (for transformed code)
    int64_t yieldedValue;       // Last yielded value
    int64_t returnValue;        // Return value when done
    int64_t inputValue;         // Value passed to next()
    bool hasError;
    int64_t error;
    std::vector<int64_t> locals; // Local variable storage
    std::mutex mutex;
};

// AsyncGenerator object (extends Generator with Promise support)
struct NovaAsyncGenerator {
    NovaGenerator* generator;
    bool isAsync;
};

// Global generator context for yield
thread_local NovaGenerator* currentGenerator = nullptr;

// ============= Iterator Result Functions =============

extern "C" void* nova_iterator_result_create(int64_t value, bool done) {
    auto* result = new IteratorResult();
    result->value = value;
    result->done = done;
    return result;
}

extern "C" int64_t nova_iterator_result_value(void* resultPtr) {
    if (!resultPtr) return 0;
    auto* result = static_cast<IteratorResult*>(resultPtr);
    return result->value;
}

extern "C" int64_t nova_iterator_result_done(void* resultPtr) {
    if (!resultPtr) return 1;  // true
    auto* result = static_cast<IteratorResult*>(resultPtr);
    return result->done ? 1 : 0;
}

// ============= Generator Functions =============

extern "C" void* nova_generator_create(void* funcPtr, int64_t initialState) {
    auto* gen = new NovaGenerator();
    gen->state = GeneratorState::CREATED;
    gen->functionPtr = funcPtr;
    gen->currentState = initialState;
    gen->yieldedValue = 0;
    gen->returnValue = 0;
    gen->inputValue = 0;
    gen->hasError = false;
    gen->error = 0;

    // Pre-allocate some local storage
    gen->locals.resize(32, 0);

    return gen;
}

// Set generator state (called by transformed code)
extern "C" void nova_generator_set_state(void* genPtr, int64_t state) {
    if (!genPtr) return;
    auto* gen = static_cast<NovaGenerator*>(genPtr);
    gen->currentState = state;
}

// Get generator state
extern "C" int64_t nova_generator_get_state(void* genPtr) {
    if (!genPtr) return -1;
    auto* gen = static_cast<NovaGenerator*>(genPtr);
    return gen->currentState;
}

// Store local variable
extern "C" void nova_generator_store_local(void* genPtr, int64_t index, int64_t value) {
    if (!genPtr) return;
    auto* gen = static_cast<NovaGenerator*>(genPtr);
    if (index >= 0 && static_cast<size_t>(index) < gen->locals.size()) {
        gen->locals[index] = value;
    } else if (index >= 0) {
        gen->locals.resize(index + 1, 0);
        gen->locals[index] = value;
    }
}

// Load local variable
extern "C" int64_t nova_generator_load_local(void* genPtr, int64_t index) {
    if (!genPtr) return 0;
    auto* gen = static_cast<NovaGenerator*>(genPtr);
    if (index >= 0 && static_cast<size_t>(index) < gen->locals.size()) {
        return gen->locals[index];
    }
    return 0;
}

// Get input value (value passed to next())
extern "C" int64_t nova_generator_get_input(void* genPtr) {
    if (!genPtr) return 0;
    auto* gen = static_cast<NovaGenerator*>(genPtr);
    return gen->inputValue;
}

// Yield a value (called by generator body)
extern "C" void nova_generator_yield(void* genPtr, int64_t value) {
    if (!genPtr) return;
    auto* gen = static_cast<NovaGenerator*>(genPtr);
    gen->yieldedValue = value;
    gen->state = GeneratorState::SUSPENDED;
}

// Mark generator as completed
extern "C" void nova_generator_complete(void* genPtr, int64_t returnValue) {
    if (!genPtr) return;
    auto* gen = static_cast<NovaGenerator*>(genPtr);
    gen->returnValue = returnValue;
    gen->state = GeneratorState::COMPLETED;
}

// Type for generator step function
typedef int64_t (*GeneratorStepFn)(void* genPtr, int64_t input);

// Generator.next(value) - advance generator
extern "C" void* nova_generator_next(void* genPtr, int64_t value) {
    if (!genPtr) {
        return nova_iterator_result_create(0, true);
    }

    auto* gen = static_cast<NovaGenerator*>(genPtr);
    std::lock_guard<std::mutex> lock(gen->mutex);

    // Already completed
    if (gen->state == GeneratorState::COMPLETED) {
        return nova_iterator_result_create(gen->returnValue, true);
    }

    // Store input value
    gen->inputValue = value;

    // Set running state
    gen->state = GeneratorState::RUNNING;

    // Set current generator for yield
    NovaGenerator* prevGenerator = currentGenerator;
    currentGenerator = gen;

    // Call the generator step function
    // The compiler now passes the actual function pointer
    if (gen->functionPtr) {
        auto stepFn = reinterpret_cast<GeneratorStepFn>(gen->functionPtr);
        stepFn(genPtr, value);

        // If state is still RUNNING after the function returns, mark as completed
        if (gen->state == GeneratorState::RUNNING) {
            gen->state = GeneratorState::COMPLETED;
        }
    } else {
        // No function pointer, mark as completed
        gen->state = GeneratorState::COMPLETED;
        gen->returnValue = 0;
    }

    // Restore previous generator
    currentGenerator = prevGenerator;

    // Return result based on state
    if (gen->state == GeneratorState::COMPLETED) {
        return nova_iterator_result_create(gen->returnValue, true);
    } else if (gen->state == GeneratorState::SUSPENDED) {
        return nova_iterator_result_create(gen->yieldedValue, false);
    }

    // Shouldn't reach here
    return nova_iterator_result_create(0, true);
}

// Generator.return(value) - complete generator with value
extern "C" void* nova_generator_return(void* genPtr, int64_t value) {
    if (!genPtr) {
        return nova_iterator_result_create(value, true);
    }

    auto* gen = static_cast<NovaGenerator*>(genPtr);
    std::lock_guard<std::mutex> lock(gen->mutex);

    gen->returnValue = value;
    gen->state = GeneratorState::COMPLETED;

    return nova_iterator_result_create(value, true);
}

// Generator.throw(error) - throw error into generator
extern "C" void* nova_generator_throw(void* genPtr, int64_t error) {
    if (!genPtr) {
        return nova_iterator_result_create(0, true);
    }

    auto* gen = static_cast<NovaGenerator*>(genPtr);
    std::lock_guard<std::mutex> lock(gen->mutex);

    gen->hasError = true;
    gen->error = error;
    gen->state = GeneratorState::COMPLETED;

    return nova_iterator_result_create(0, true);
}

// ============= AsyncGenerator Functions =============

extern "C" void* nova_async_generator_create(void* funcPtr, int64_t initialState) {
    auto* asyncGen = new NovaAsyncGenerator();
    asyncGen->generator = static_cast<NovaGenerator*>(nova_generator_create(funcPtr, initialState));
    asyncGen->isAsync = true;
    return asyncGen;
}

// Forward declaration for Promise functions (for future full async support)
// extern "C" void* nova_promise_create();
// extern "C" void nova_promise_fulfill(void* promisePtr, int64_t value);
// extern "C" void nova_promise_reject_internal(void* promisePtr, int64_t reason);

// Helper to wrap iterator result in promise (for future full async support)
// static void* wrapInPromise(void* iterResult) {
//     void* promise = nova_promise_create();
//     int64_t value = nova_iterator_result_value(iterResult);
//     nova_promise_fulfill(promise, value);
//     return promise;
// }

// AsyncGenerator.next(value) - returns IteratorResult (synchronous compilation)
// Note: Full async support would return Promise<IteratorResult>
extern "C" void* nova_async_generator_next(void* genPtr, int64_t value) {
    if (!genPtr) {
        return nova_iterator_result_create(0, true);
    }

    auto* asyncGen = static_cast<NovaAsyncGenerator*>(genPtr);
    // For synchronous compilation, return IteratorResult directly
    return nova_generator_next(asyncGen->generator, value);
}

// AsyncGenerator.return(value) - returns IteratorResult (synchronous compilation)
extern "C" void* nova_async_generator_return(void* genPtr, int64_t value) {
    if (!genPtr) {
        return nova_iterator_result_create(value, true);
    }

    auto* asyncGen = static_cast<NovaAsyncGenerator*>(genPtr);
    return nova_generator_return(asyncGen->generator, value);
}

// AsyncGenerator.throw(error) - returns IteratorResult (synchronous compilation)
extern "C" void* nova_async_generator_throw(void* genPtr, int64_t error) {
    if (!genPtr) {
        return nova_iterator_result_create(0, true);
    }

    auto* asyncGen = static_cast<NovaAsyncGenerator*>(genPtr);
    return nova_generator_throw(asyncGen->generator, error);
}

// ============= Symbol.iterator support =============

extern "C" void* nova_get_iterator(void* obj) {
    // For generators, the generator itself is its own iterator
    return obj;
}

// ============= for-of loop support =============

extern "C" bool nova_iterator_has_next(void* iterResult) {
    return !nova_iterator_result_done(iterResult);
}
