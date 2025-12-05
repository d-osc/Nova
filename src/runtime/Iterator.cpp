// Iterator Helpers Runtime (ES2025)
// Debug mode disabled
#define NOVA_DEBUG 0
// Implements the Iterator Helpers proposal for Nova AOT compiler

#include <iostream>
#include <cstdint>
#include <cstring>
#include <vector>
#include <functional>


extern "C" {
    // Array runtime functions
    void* nova_create_array(int64_t size);
    void nova_array_push(void* arr, int64_t value);
    int64_t nova_value_array_length(void* arr);
    int64_t nova_value_array_at(void* arr, int64_t index);

    // Generator/Iterator result functions
    void* nova_iterator_result_create(int64_t value, bool done);
    int64_t nova_iterator_result_value(void* resultPtr);
    int64_t nova_iterator_result_done(void* resultPtr);
}

// ============= Iterator Object Structure =============

struct NovaIterator {
    void* source;           // Source iterable (array, generator, etc.)
    int64_t currentIndex;   // Current position for array iterators
    int64_t length;         // Length for array iterators
    bool isArray;           // True if iterating over array
    bool isDone;            // True if iterator is exhausted
    void* nextFunc;         // Custom next function (for wrapped iterators)

    // For helper methods (map, filter, etc.)
    void* transformFunc;    // Transformation function
    int64_t transformType;  // 0=none, 1=map, 2=filter, 3=take, 4=drop, 5=flatMap
    int64_t transformArg;   // Argument for take/drop (count)
    int64_t dropCount;      // Remaining items to drop
    NovaIterator* innerIter; // Inner iterator for chained operations
};

// ============= Iterator Creation =============

// Iterator.from(iterable) - Create iterator from array or iterable
extern "C" void* nova_iterator_from(void* iterable) {
    if(NOVA_DEBUG) std::cerr << "DEBUG: nova_iterator_from called" << std::endl;

    auto* iter = new NovaIterator();
    iter->source = iterable;
    iter->currentIndex = 0;
    iter->length = nova_value_array_length(iterable);
    iter->isArray = true;
    iter->isDone = false;
    iter->nextFunc = nullptr;
    iter->transformFunc = nullptr;
    iter->transformType = 0;
    iter->transformArg = 0;
    iter->dropCount = 0;
    iter->innerIter = nullptr;

    return iter;
}

// Create an empty/done iterator
extern "C" void* nova_iterator_create_empty() {
    auto* iter = new NovaIterator();
    iter->source = nullptr;
    iter->currentIndex = 0;
    iter->length = 0;
    iter->isArray = false;
    iter->isDone = true;
    iter->nextFunc = nullptr;
    iter->transformFunc = nullptr;
    iter->transformType = 0;
    iter->transformArg = 0;
    iter->dropCount = 0;
    iter->innerIter = nullptr;

    return iter;
}

// ============= Iterator.prototype.next() =============

extern "C" void* nova_iterator_next(void* iterPtr) {
    if (!iterPtr) {
        return nova_iterator_result_create(0, true);
    }

    auto* iter = static_cast<NovaIterator*>(iterPtr);

    if (iter->isDone) {
        return nova_iterator_result_create(0, true);
    }

    // Handle array iteration
    if (iter->isArray && iter->source) {
        if (iter->currentIndex >= iter->length) {
            iter->isDone = true;
            return nova_iterator_result_create(0, true);
        }

        int64_t value = nova_value_array_at(iter->source, iter->currentIndex);
        iter->currentIndex++;

        return nova_iterator_result_create(value, false);
    }

    // Default: done
    iter->isDone = true;
    return nova_iterator_result_create(0, true);
}

// ============= Iterator.prototype.map(fn) =============

// Internal struct for map iterator
struct MapIterator {
    NovaIterator* source;
    void* mapFunc;
};

extern "C" void* nova_iterator_map(void* iterPtr, void* mapFunc) {
    if(NOVA_DEBUG) std::cerr << "DEBUG: nova_iterator_map called" << std::endl;

    if (!iterPtr) {
        return nova_iterator_create_empty();
    }

    auto* sourceIter = static_cast<NovaIterator*>(iterPtr);

    auto* newIter = new NovaIterator();
    newIter->source = sourceIter->source;
    newIter->currentIndex = sourceIter->currentIndex;
    newIter->length = sourceIter->length;
    newIter->isArray = sourceIter->isArray;
    newIter->isDone = sourceIter->isDone;
    newIter->nextFunc = nullptr;
    newIter->transformFunc = mapFunc;
    newIter->transformType = 1; // map
    newIter->transformArg = 0;
    newIter->dropCount = 0;
    newIter->innerIter = sourceIter;

    return newIter;
}

// ============= Iterator.prototype.filter(fn) =============

extern "C" void* nova_iterator_filter(void* iterPtr, void* filterFunc) {
    if(NOVA_DEBUG) std::cerr << "DEBUG: nova_iterator_filter called" << std::endl;

    if (!iterPtr) {
        return nova_iterator_create_empty();
    }

    auto* sourceIter = static_cast<NovaIterator*>(iterPtr);

    auto* newIter = new NovaIterator();
    newIter->source = sourceIter->source;
    newIter->currentIndex = sourceIter->currentIndex;
    newIter->length = sourceIter->length;
    newIter->isArray = sourceIter->isArray;
    newIter->isDone = sourceIter->isDone;
    newIter->nextFunc = nullptr;
    newIter->transformFunc = filterFunc;
    newIter->transformType = 2; // filter
    newIter->transformArg = 0;
    newIter->dropCount = 0;
    newIter->innerIter = sourceIter;

    return newIter;
}

// ============= Iterator.prototype.take(n) =============

extern "C" void* nova_iterator_take(void* iterPtr, int64_t count) {
    if(NOVA_DEBUG) std::cerr << "DEBUG: nova_iterator_take called with count=" << count << std::endl;

    if (!iterPtr || count <= 0) {
        return nova_iterator_create_empty();
    }

    auto* sourceIter = static_cast<NovaIterator*>(iterPtr);

    auto* newIter = new NovaIterator();
    newIter->source = sourceIter->source;
    newIter->currentIndex = sourceIter->currentIndex;
    newIter->length = sourceIter->length;
    newIter->isArray = sourceIter->isArray;
    newIter->isDone = sourceIter->isDone;
    newIter->nextFunc = nullptr;
    newIter->transformFunc = nullptr;
    newIter->transformType = 3; // take
    newIter->transformArg = count;
    newIter->dropCount = 0;
    newIter->innerIter = sourceIter;

    return newIter;
}

// ============= Iterator.prototype.drop(n) =============

extern "C" void* nova_iterator_drop(void* iterPtr, int64_t count) {
    if(NOVA_DEBUG) std::cerr << "DEBUG: nova_iterator_drop called with count=" << count << std::endl;

    if (!iterPtr) {
        return nova_iterator_create_empty();
    }

    auto* sourceIter = static_cast<NovaIterator*>(iterPtr);

    auto* newIter = new NovaIterator();
    newIter->source = sourceIter->source;
    newIter->currentIndex = sourceIter->currentIndex;
    newIter->length = sourceIter->length;
    newIter->isArray = sourceIter->isArray;
    newIter->isDone = sourceIter->isDone;
    newIter->nextFunc = nullptr;
    newIter->transformFunc = nullptr;
    newIter->transformType = 4; // drop
    newIter->transformArg = count;
    newIter->dropCount = count; // Items remaining to drop
    newIter->innerIter = sourceIter;

    return newIter;
}

// ============= Iterator.prototype.flatMap(fn) =============

extern "C" void* nova_iterator_flatmap(void* iterPtr, void* flatMapFunc) {
    if(NOVA_DEBUG) std::cerr << "DEBUG: nova_iterator_flatmap called" << std::endl;

    if (!iterPtr) {
        return nova_iterator_create_empty();
    }

    auto* sourceIter = static_cast<NovaIterator*>(iterPtr);

    auto* newIter = new NovaIterator();
    newIter->source = sourceIter->source;
    newIter->currentIndex = sourceIter->currentIndex;
    newIter->length = sourceIter->length;
    newIter->isArray = sourceIter->isArray;
    newIter->isDone = sourceIter->isDone;
    newIter->nextFunc = nullptr;
    newIter->transformFunc = flatMapFunc;
    newIter->transformType = 5; // flatMap
    newIter->transformArg = 0;
    newIter->dropCount = 0;
    newIter->innerIter = sourceIter;

    return newIter;
}

// ============= Iterator.prototype.toArray() =============

extern "C" void* nova_iterator_toarray(void* iterPtr) {
    if(NOVA_DEBUG) std::cerr << "DEBUG: nova_iterator_toarray called" << std::endl;

    void* result = nova_create_array(0);

    if (!iterPtr) {
        return result;
    }

    auto* iter = static_cast<NovaIterator*>(iterPtr);

    // For simple array iterators, copy remaining elements
    if (iter->isArray && iter->source && iter->transformType == 0) {
        for (int64_t i = iter->currentIndex; i < iter->length; i++) {
            int64_t value = nova_value_array_at(iter->source, i);
            nova_array_push(result, value);
        }
        iter->isDone = true;
        return result;
    }

    // For transformed iterators, iterate through
    while (!iter->isDone) {
        void* nextResult = nova_iterator_next(iter);
        if (nova_iterator_result_done(nextResult)) {
            break;
        }
        int64_t value = nova_iterator_result_value(nextResult);
        nova_array_push(result, value);
    }

    return result;
}

// ============= Iterator.prototype.reduce(fn, initialValue) =============

extern "C" int64_t nova_iterator_reduce(void* iterPtr, [[maybe_unused]] void* reduceFunc, int64_t initialValue) {
    if(NOVA_DEBUG) std::cerr << "DEBUG: nova_iterator_reduce called" << std::endl;

    if (!iterPtr) {
        return initialValue;
    }

    auto* iter = static_cast<NovaIterator*>(iterPtr);
    int64_t accumulator = initialValue;

    // For simple array iterators
    if (iter->isArray && iter->source && iter->transformType == 0) {
        for (int64_t i = iter->currentIndex; i < iter->length; i++) {
            int64_t value = nova_value_array_at(iter->source, i);
            // In a real implementation, we'd call reduceFunc(accumulator, value)
            // For now, just sum the values as a basic reduce
            accumulator += value;
        }
        iter->isDone = true;
    }

    return accumulator;
}

// ============= Iterator.prototype.forEach(fn) =============

extern "C" void nova_iterator_foreach(void* iterPtr, [[maybe_unused]] void* forEachFunc) {
    if(NOVA_DEBUG) std::cerr << "DEBUG: nova_iterator_foreach called" << std::endl;

    if (!iterPtr) {
        return;
    }

    auto* iter = static_cast<NovaIterator*>(iterPtr);

    // For simple array iterators
    if (iter->isArray && iter->source && iter->transformType == 0) {
        for (int64_t i = iter->currentIndex; i < iter->length; i++) {
            int64_t value = nova_value_array_at(iter->source, i);
            // In a real implementation, we'd call forEachFunc(value, i)
            std::cerr << "  forEach value: " << value << std::endl;
        }
        iter->isDone = true;
    }
}

// ============= Iterator.prototype.some(fn) =============

extern "C" int64_t nova_iterator_some(void* iterPtr, [[maybe_unused]] void* someFunc) {
    if(NOVA_DEBUG) std::cerr << "DEBUG: nova_iterator_some called" << std::endl;

    if (!iterPtr) {
        return 0; // false
    }

    auto* iter = static_cast<NovaIterator*>(iterPtr);

    // For simple array iterators - return true if any element exists
    if (iter->isArray && iter->source && iter->transformType == 0) {
        bool hasAny = iter->currentIndex < iter->length;
        iter->isDone = true;
        return hasAny ? 1 : 0;
    }

    return 0;
}

// ============= Iterator.prototype.every(fn) =============

extern "C" int64_t nova_iterator_every(void* iterPtr, [[maybe_unused]] void* everyFunc) {
    if(NOVA_DEBUG) std::cerr << "DEBUG: nova_iterator_every called" << std::endl;

    if (!iterPtr) {
        return 1; // true (vacuous truth for empty iterator)
    }

    auto* iter = static_cast<NovaIterator*>(iterPtr);

    // For simple array iterators - without callback, return true if any elements
    if (iter->isArray && iter->source && iter->transformType == 0) {
        iter->isDone = true;
        return 1; // Without actual predicate testing, assume true
    }

    return 1;
}

// ============= Iterator.prototype.find(fn) =============

extern "C" int64_t nova_iterator_find(void* iterPtr, [[maybe_unused]] void* findFunc) {
    if(NOVA_DEBUG) std::cerr << "DEBUG: nova_iterator_find called" << std::endl;

    if (!iterPtr) {
        return 0; // undefined
    }

    auto* iter = static_cast<NovaIterator*>(iterPtr);

    // For simple array iterators - return first element
    if (iter->isArray && iter->source && iter->transformType == 0) {
        if (iter->currentIndex < iter->length) {
            int64_t value = nova_value_array_at(iter->source, iter->currentIndex);
            iter->isDone = true;
            return value;
        }
    }

    iter->isDone = true;
    return 0; // undefined
}

// ============= Iterator[Symbol.iterator]() =============

extern "C" void* nova_iterator_symbol_iterator(void* iterPtr) {
    // Iterator[Symbol.iterator]() returns itself
    return iterPtr;
}

// ============= Iterator.prototype.return() =============

extern "C" void* nova_iterator_return(void* iterPtr, int64_t value) {
    if (!iterPtr) {
        return nova_iterator_result_create(value, true);
    }

    auto* iter = static_cast<NovaIterator*>(iterPtr);
    iter->isDone = true;

    return nova_iterator_result_create(value, true);
}

// ============= Iterator.prototype.throw() =============

extern "C" void* nova_iterator_throw(void* iterPtr, [[maybe_unused]] void* error) {
    if (!iterPtr) {
        return nova_iterator_result_create(0, true);
    }

    auto* iter = static_cast<NovaIterator*>(iterPtr);
    iter->isDone = true;

    // In a real implementation, this would throw the error
    std::cerr << "Iterator.throw() called - closing iterator" << std::endl;

    return nova_iterator_result_create(0, true);
}
