// Nova Runtime - SharedArrayBuffer and Atomics Implementation
// Implements ES2017 SharedArrayBuffer and Atomics API for thread-safe operations

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

extern "C" {

// ============================================================================
// SharedArrayBuffer Structure
// ============================================================================
struct NovaSharedArrayBuffer {
    uint8_t* data;          // Raw byte data (shared memory)
    int64_t byteLength;     // Length in bytes
    std::atomic<int32_t> refCount;  // Reference count for shared ownership
    bool growable;          // Whether buffer can grow (ES2024)
    int64_t maxByteLength;  // Maximum byte length if growable
};

// ============================================================================
// SharedArrayBuffer Creation
// ============================================================================

// new SharedArrayBuffer(length)
void* nova_sharedarraybuffer_create(int64_t byteLength) {
    if (byteLength < 0) byteLength = 0;

    NovaSharedArrayBuffer* buffer = new NovaSharedArrayBuffer();
    buffer->byteLength = byteLength;
    buffer->refCount.store(1);
    buffer->growable = false;
    buffer->maxByteLength = byteLength;

    if (byteLength > 0) {
        buffer->data = static_cast<uint8_t*>(calloc(byteLength, 1)); // Zero-initialized
    } else {
        buffer->data = nullptr;
    }

    return buffer;
}

// new SharedArrayBuffer(length, { maxByteLength }) - ES2024 growable
void* nova_sharedarraybuffer_create_growable(int64_t byteLength, int64_t maxByteLength) {
    if (byteLength < 0) byteLength = 0;
    if (maxByteLength < byteLength) maxByteLength = byteLength;

    NovaSharedArrayBuffer* buffer = new NovaSharedArrayBuffer();
    buffer->byteLength = byteLength;
    buffer->refCount.store(1);
    buffer->growable = true;
    buffer->maxByteLength = maxByteLength;

    if (maxByteLength > 0) {
        // Allocate max size for potential growth
        buffer->data = static_cast<uint8_t*>(calloc(maxByteLength, 1));
    } else {
        buffer->data = nullptr;
    }

    return buffer;
}

// SharedArrayBuffer.byteLength getter
int64_t nova_sharedarraybuffer_byteLength(void* bufferPtr) {
    if (!bufferPtr) return 0;
    NovaSharedArrayBuffer* buffer = static_cast<NovaSharedArrayBuffer*>(bufferPtr);
    return buffer->byteLength;
}

// SharedArrayBuffer.maxByteLength getter (ES2024)
int64_t nova_sharedarraybuffer_maxByteLength(void* bufferPtr) {
    if (!bufferPtr) return 0;
    NovaSharedArrayBuffer* buffer = static_cast<NovaSharedArrayBuffer*>(bufferPtr);
    return buffer->maxByteLength;
}

// SharedArrayBuffer.growable getter (ES2024)
int64_t nova_sharedarraybuffer_growable(void* bufferPtr) {
    if (!bufferPtr) return 0;
    NovaSharedArrayBuffer* buffer = static_cast<NovaSharedArrayBuffer*>(bufferPtr);
    return buffer->growable ? 1 : 0;
}

// SharedArrayBuffer.prototype.grow(newLength) - ES2024
int64_t nova_sharedarraybuffer_grow(void* bufferPtr, int64_t newLength) {
    if (!bufferPtr) return 0;
    NovaSharedArrayBuffer* buffer = static_cast<NovaSharedArrayBuffer*>(bufferPtr);

    if (!buffer->growable) return 0;  // TypeError in JS
    if (newLength < buffer->byteLength) return 0;  // RangeError in JS
    if (newLength > buffer->maxByteLength) return 0;  // RangeError in JS

    // Atomically update byteLength (memory is already allocated)
    buffer->byteLength = newLength;
    return 1;
}

// SharedArrayBuffer.prototype.slice(begin, end)
void* nova_sharedarraybuffer_slice(void* bufferPtr, int64_t begin, int64_t end) {
    if (!bufferPtr) return nova_sharedarraybuffer_create(0);
    NovaSharedArrayBuffer* buffer = static_cast<NovaSharedArrayBuffer*>(bufferPtr);

    int64_t len = buffer->byteLength;

    // Handle negative indices
    if (begin < 0) begin = (len + begin > 0) ? len + begin : 0;
    else if (begin > len) begin = len;

    if (end < 0) end = (len + end > 0) ? len + end : 0;
    else if (end > len) end = len;

    int64_t newLen = (end > begin) ? end - begin : 0;

    // Create new SharedArrayBuffer (not a view, actual copy)
    NovaSharedArrayBuffer* newBuffer = static_cast<NovaSharedArrayBuffer*>(
        nova_sharedarraybuffer_create(newLen));

    if (newLen > 0 && buffer->data) {
        memcpy(newBuffer->data, buffer->data + begin, newLen);
    }

    return newBuffer;
}

// Get raw data pointer (for TypedArray views)
uint8_t* nova_sharedarraybuffer_data(void* bufferPtr) {
    if (!bufferPtr) return nullptr;
    NovaSharedArrayBuffer* buffer = static_cast<NovaSharedArrayBuffer*>(bufferPtr);
    return buffer->data;
}

// Free SharedArrayBuffer (decrements refcount, frees if zero)
void nova_sharedarraybuffer_free(void* bufferPtr) {
    if (!bufferPtr) return;
    NovaSharedArrayBuffer* buffer = static_cast<NovaSharedArrayBuffer*>(bufferPtr);

    if (buffer->refCount.fetch_sub(1) == 1) {
        if (buffer->data) free(buffer->data);
        delete buffer;
    }
}

// ============================================================================
// Atomics - Wait/Notify Support
// ============================================================================

// Global wait list for Atomics.wait/notify
struct WaitEntry {
    std::mutex mutex;
    std::condition_variable cv;
    bool notified;
};

static std::mutex waitListMutex;
static std::unordered_map<uintptr_t, std::vector<WaitEntry*>> waitLists;

// ============================================================================
// Atomics Static Methods
// ============================================================================

// Atomics.add(typedArray, index, value) - atomically add and return old value
int64_t nova_atomics_add_i32(void* arrayPtr, int64_t index, int64_t value) {
    if (!arrayPtr) return 0;

    // Get the underlying data pointer from TypedArray
    struct NovaTypedArray {
        void* buffer;
        uint8_t* data;
        int64_t byteOffset;
        int64_t byteLength;
        int64_t length;
        int64_t bytesPerElement;
        int64_t typeId;
    };

    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr->data || index < 0 || index >= arr->length) return 0;

    std::atomic<int32_t>* atomicPtr = reinterpret_cast<std::atomic<int32_t>*>(
        arr->data + index * sizeof(int32_t));

    return atomicPtr->fetch_add(static_cast<int32_t>(value), std::memory_order_seq_cst);
}

int64_t nova_atomics_add_i64(void* arrayPtr, int64_t index, int64_t value) {
    if (!arrayPtr) return 0;

    struct NovaTypedArray {
        void* buffer;
        uint8_t* data;
        int64_t byteOffset;
        int64_t byteLength;
        int64_t length;
        int64_t bytesPerElement;
        int64_t typeId;
    };

    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr->data || index < 0 || index >= arr->length) return 0;

    std::atomic<int64_t>* atomicPtr = reinterpret_cast<std::atomic<int64_t>*>(
        arr->data + index * sizeof(int64_t));

    return atomicPtr->fetch_add(value, std::memory_order_seq_cst);
}

// Atomics.sub(typedArray, index, value) - atomically subtract and return old value
int64_t nova_atomics_sub_i32(void* arrayPtr, int64_t index, int64_t value) {
    if (!arrayPtr) return 0;

    struct NovaTypedArray {
        void* buffer;
        uint8_t* data;
        int64_t byteOffset;
        int64_t byteLength;
        int64_t length;
        int64_t bytesPerElement;
        int64_t typeId;
    };

    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr->data || index < 0 || index >= arr->length) return 0;

    std::atomic<int32_t>* atomicPtr = reinterpret_cast<std::atomic<int32_t>*>(
        arr->data + index * sizeof(int32_t));

    return atomicPtr->fetch_sub(static_cast<int32_t>(value), std::memory_order_seq_cst);
}

int64_t nova_atomics_sub_i64(void* arrayPtr, int64_t index, int64_t value) {
    if (!arrayPtr) return 0;

    struct NovaTypedArray {
        void* buffer;
        uint8_t* data;
        int64_t byteOffset;
        int64_t byteLength;
        int64_t length;
        int64_t bytesPerElement;
        int64_t typeId;
    };

    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr->data || index < 0 || index >= arr->length) return 0;

    std::atomic<int64_t>* atomicPtr = reinterpret_cast<std::atomic<int64_t>*>(
        arr->data + index * sizeof(int64_t));

    return atomicPtr->fetch_sub(value, std::memory_order_seq_cst);
}

// Atomics.and(typedArray, index, value) - atomically AND and return old value
int64_t nova_atomics_and_i32(void* arrayPtr, int64_t index, int64_t value) {
    if (!arrayPtr) return 0;

    struct NovaTypedArray {
        void* buffer;
        uint8_t* data;
        int64_t byteOffset;
        int64_t byteLength;
        int64_t length;
        int64_t bytesPerElement;
        int64_t typeId;
    };

    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr->data || index < 0 || index >= arr->length) return 0;

    std::atomic<int32_t>* atomicPtr = reinterpret_cast<std::atomic<int32_t>*>(
        arr->data + index * sizeof(int32_t));

    return atomicPtr->fetch_and(static_cast<int32_t>(value), std::memory_order_seq_cst);
}

int64_t nova_atomics_and_i64(void* arrayPtr, int64_t index, int64_t value) {
    if (!arrayPtr) return 0;

    struct NovaTypedArray {
        void* buffer;
        uint8_t* data;
        int64_t byteOffset;
        int64_t byteLength;
        int64_t length;
        int64_t bytesPerElement;
        int64_t typeId;
    };

    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr->data || index < 0 || index >= arr->length) return 0;

    std::atomic<int64_t>* atomicPtr = reinterpret_cast<std::atomic<int64_t>*>(
        arr->data + index * sizeof(int64_t));

    return atomicPtr->fetch_and(value, std::memory_order_seq_cst);
}

// Atomics.or(typedArray, index, value) - atomically OR and return old value
int64_t nova_atomics_or_i32(void* arrayPtr, int64_t index, int64_t value) {
    if (!arrayPtr) return 0;

    struct NovaTypedArray {
        void* buffer;
        uint8_t* data;
        int64_t byteOffset;
        int64_t byteLength;
        int64_t length;
        int64_t bytesPerElement;
        int64_t typeId;
    };

    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr->data || index < 0 || index >= arr->length) return 0;

    std::atomic<int32_t>* atomicPtr = reinterpret_cast<std::atomic<int32_t>*>(
        arr->data + index * sizeof(int32_t));

    return atomicPtr->fetch_or(static_cast<int32_t>(value), std::memory_order_seq_cst);
}

int64_t nova_atomics_or_i64(void* arrayPtr, int64_t index, int64_t value) {
    if (!arrayPtr) return 0;

    struct NovaTypedArray {
        void* buffer;
        uint8_t* data;
        int64_t byteOffset;
        int64_t byteLength;
        int64_t length;
        int64_t bytesPerElement;
        int64_t typeId;
    };

    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr->data || index < 0 || index >= arr->length) return 0;

    std::atomic<int64_t>* atomicPtr = reinterpret_cast<std::atomic<int64_t>*>(
        arr->data + index * sizeof(int64_t));

    return atomicPtr->fetch_or(value, std::memory_order_seq_cst);
}

// Atomics.xor(typedArray, index, value) - atomically XOR and return old value
int64_t nova_atomics_xor_i32(void* arrayPtr, int64_t index, int64_t value) {
    if (!arrayPtr) return 0;

    struct NovaTypedArray {
        void* buffer;
        uint8_t* data;
        int64_t byteOffset;
        int64_t byteLength;
        int64_t length;
        int64_t bytesPerElement;
        int64_t typeId;
    };

    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr->data || index < 0 || index >= arr->length) return 0;

    std::atomic<int32_t>* atomicPtr = reinterpret_cast<std::atomic<int32_t>*>(
        arr->data + index * sizeof(int32_t));

    return atomicPtr->fetch_xor(static_cast<int32_t>(value), std::memory_order_seq_cst);
}

int64_t nova_atomics_xor_i64(void* arrayPtr, int64_t index, int64_t value) {
    if (!arrayPtr) return 0;

    struct NovaTypedArray {
        void* buffer;
        uint8_t* data;
        int64_t byteOffset;
        int64_t byteLength;
        int64_t length;
        int64_t bytesPerElement;
        int64_t typeId;
    };

    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr->data || index < 0 || index >= arr->length) return 0;

    std::atomic<int64_t>* atomicPtr = reinterpret_cast<std::atomic<int64_t>*>(
        arr->data + index * sizeof(int64_t));

    return atomicPtr->fetch_xor(value, std::memory_order_seq_cst);
}

// Atomics.load(typedArray, index) - atomically read value
int64_t nova_atomics_load_i32(void* arrayPtr, int64_t index) {
    if (!arrayPtr) return 0;

    struct NovaTypedArray {
        void* buffer;
        uint8_t* data;
        int64_t byteOffset;
        int64_t byteLength;
        int64_t length;
        int64_t bytesPerElement;
        int64_t typeId;
    };

    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr->data || index < 0 || index >= arr->length) return 0;

    std::atomic<int32_t>* atomicPtr = reinterpret_cast<std::atomic<int32_t>*>(
        arr->data + index * sizeof(int32_t));

    return atomicPtr->load(std::memory_order_seq_cst);
}

int64_t nova_atomics_load_i64(void* arrayPtr, int64_t index) {
    if (!arrayPtr) return 0;

    struct NovaTypedArray {
        void* buffer;
        uint8_t* data;
        int64_t byteOffset;
        int64_t byteLength;
        int64_t length;
        int64_t bytesPerElement;
        int64_t typeId;
    };

    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr->data || index < 0 || index >= arr->length) return 0;

    std::atomic<int64_t>* atomicPtr = reinterpret_cast<std::atomic<int64_t>*>(
        arr->data + index * sizeof(int64_t));

    return atomicPtr->load(std::memory_order_seq_cst);
}

// Atomics.store(typedArray, index, value) - atomically write value, return value
int64_t nova_atomics_store_i32(void* arrayPtr, int64_t index, int64_t value) {
    if (!arrayPtr) return value;

    struct NovaTypedArray {
        void* buffer;
        uint8_t* data;
        int64_t byteOffset;
        int64_t byteLength;
        int64_t length;
        int64_t bytesPerElement;
        int64_t typeId;
    };

    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr->data || index < 0 || index >= arr->length) return value;

    std::atomic<int32_t>* atomicPtr = reinterpret_cast<std::atomic<int32_t>*>(
        arr->data + index * sizeof(int32_t));

    atomicPtr->store(static_cast<int32_t>(value), std::memory_order_seq_cst);
    return value;
}

int64_t nova_atomics_store_i64(void* arrayPtr, int64_t index, int64_t value) {
    if (!arrayPtr) return value;

    struct NovaTypedArray {
        void* buffer;
        uint8_t* data;
        int64_t byteOffset;
        int64_t byteLength;
        int64_t length;
        int64_t bytesPerElement;
        int64_t typeId;
    };

    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr->data || index < 0 || index >= arr->length) return value;

    std::atomic<int64_t>* atomicPtr = reinterpret_cast<std::atomic<int64_t>*>(
        arr->data + index * sizeof(int64_t));

    atomicPtr->store(value, std::memory_order_seq_cst);
    return value;
}

// Atomics.exchange(typedArray, index, value) - atomically swap and return old value
int64_t nova_atomics_exchange_i32(void* arrayPtr, int64_t index, int64_t value) {
    if (!arrayPtr) return 0;

    struct NovaTypedArray {
        void* buffer;
        uint8_t* data;
        int64_t byteOffset;
        int64_t byteLength;
        int64_t length;
        int64_t bytesPerElement;
        int64_t typeId;
    };

    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr->data || index < 0 || index >= arr->length) return 0;

    std::atomic<int32_t>* atomicPtr = reinterpret_cast<std::atomic<int32_t>*>(
        arr->data + index * sizeof(int32_t));

    return atomicPtr->exchange(static_cast<int32_t>(value), std::memory_order_seq_cst);
}

int64_t nova_atomics_exchange_i64(void* arrayPtr, int64_t index, int64_t value) {
    if (!arrayPtr) return 0;

    struct NovaTypedArray {
        void* buffer;
        uint8_t* data;
        int64_t byteOffset;
        int64_t byteLength;
        int64_t length;
        int64_t bytesPerElement;
        int64_t typeId;
    };

    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr->data || index < 0 || index >= arr->length) return 0;

    std::atomic<int64_t>* atomicPtr = reinterpret_cast<std::atomic<int64_t>*>(
        arr->data + index * sizeof(int64_t));

    return atomicPtr->exchange(value, std::memory_order_seq_cst);
}

// Atomics.compareExchange(typedArray, index, expectedValue, replacementValue)
// Returns old value
int64_t nova_atomics_compareExchange_i32(void* arrayPtr, int64_t index,
                                          int64_t expectedValue, int64_t replacementValue) {
    if (!arrayPtr) return 0;

    struct NovaTypedArray {
        void* buffer;
        uint8_t* data;
        int64_t byteOffset;
        int64_t byteLength;
        int64_t length;
        int64_t bytesPerElement;
        int64_t typeId;
    };

    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr->data || index < 0 || index >= arr->length) return 0;

    std::atomic<int32_t>* atomicPtr = reinterpret_cast<std::atomic<int32_t>*>(
        arr->data + index * sizeof(int32_t));

    int32_t expected = static_cast<int32_t>(expectedValue);
    atomicPtr->compare_exchange_strong(expected, static_cast<int32_t>(replacementValue),
                                       std::memory_order_seq_cst);
    return expected;  // Returns the original value (old value)
}

int64_t nova_atomics_compareExchange_i64(void* arrayPtr, int64_t index,
                                          int64_t expectedValue, int64_t replacementValue) {
    if (!arrayPtr) return 0;

    struct NovaTypedArray {
        void* buffer;
        uint8_t* data;
        int64_t byteOffset;
        int64_t byteLength;
        int64_t length;
        int64_t bytesPerElement;
        int64_t typeId;
    };

    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr->data || index < 0 || index >= arr->length) return 0;

    std::atomic<int64_t>* atomicPtr = reinterpret_cast<std::atomic<int64_t>*>(
        arr->data + index * sizeof(int64_t));

    int64_t expected = expectedValue;
    atomicPtr->compare_exchange_strong(expected, replacementValue,
                                       std::memory_order_seq_cst);
    return expected;  // Returns the original value (old value)
}

// Atomics.isLockFree(size) - check if atomic operations of given byte size are lock-free
int64_t nova_atomics_isLockFree(int64_t size) {
    switch (size) {
        case 1: return std::atomic<int8_t>{}.is_lock_free() ? 1 : 0;
        case 2: return std::atomic<int16_t>{}.is_lock_free() ? 1 : 0;
        case 4: return std::atomic<int32_t>{}.is_lock_free() ? 1 : 0;
        case 8: return std::atomic<int64_t>{}.is_lock_free() ? 1 : 0;
        default: return 0;
    }
}

// Atomics.wait(typedArray, index, value, timeout)
// Returns: 0 = "ok", 1 = "not-equal", 2 = "timed-out"
int64_t nova_atomics_wait_i32(void* arrayPtr, int64_t index, int64_t value, int64_t timeout) {
    if (!arrayPtr) return 1;  // not-equal

    struct NovaTypedArray {
        void* buffer;
        uint8_t* data;
        int64_t byteOffset;
        int64_t byteLength;
        int64_t length;
        int64_t bytesPerElement;
        int64_t typeId;
    };

    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr->data || index < 0 || index >= arr->length) return 1;

    std::atomic<int32_t>* atomicPtr = reinterpret_cast<std::atomic<int32_t>*>(
        arr->data + index * sizeof(int32_t));

    // Check if current value equals expected value
    if (atomicPtr->load(std::memory_order_seq_cst) != static_cast<int32_t>(value)) {
        return 1;  // "not-equal"
    }

    // Create wait entry
    WaitEntry* entry = new WaitEntry();
    entry->notified = false;

    uintptr_t address = reinterpret_cast<uintptr_t>(atomicPtr);

    {
        std::lock_guard<std::mutex> lock(waitListMutex);
        waitLists[address].push_back(entry);
    }

    std::unique_lock<std::mutex> lock(entry->mutex);

    int64_t result;
    if (timeout < 0) {
        // Infinite wait
        entry->cv.wait(lock, [entry] { return entry->notified; });
        result = 0;  // "ok"
    } else {
        // Timed wait
        auto duration = std::chrono::milliseconds(timeout);
        if (entry->cv.wait_for(lock, duration, [entry] { return entry->notified; })) {
            result = 0;  // "ok"
        } else {
            result = 2;  // "timed-out"
        }
    }

    // Remove from wait list
    {
        std::lock_guard<std::mutex> listLock(waitListMutex);
        auto& list = waitLists[address];
        list.erase(std::remove(list.begin(), list.end(), entry), list.end());
        if (list.empty()) {
            waitLists.erase(address);
        }
    }

    delete entry;
    return result;
}

int64_t nova_atomics_wait_i64(void* arrayPtr, int64_t index, int64_t value, int64_t timeout) {
    if (!arrayPtr) return 1;  // not-equal

    struct NovaTypedArray {
        void* buffer;
        uint8_t* data;
        int64_t byteOffset;
        int64_t byteLength;
        int64_t length;
        int64_t bytesPerElement;
        int64_t typeId;
    };

    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr->data || index < 0 || index >= arr->length) return 1;

    std::atomic<int64_t>* atomicPtr = reinterpret_cast<std::atomic<int64_t>*>(
        arr->data + index * sizeof(int64_t));

    // Check if current value equals expected value
    if (atomicPtr->load(std::memory_order_seq_cst) != value) {
        return 1;  // "not-equal"
    }

    // Create wait entry
    WaitEntry* entry = new WaitEntry();
    entry->notified = false;

    uintptr_t address = reinterpret_cast<uintptr_t>(atomicPtr);

    {
        std::lock_guard<std::mutex> lock(waitListMutex);
        waitLists[address].push_back(entry);
    }

    std::unique_lock<std::mutex> lock(entry->mutex);

    int64_t result;
    if (timeout < 0) {
        // Infinite wait
        entry->cv.wait(lock, [entry] { return entry->notified; });
        result = 0;  // "ok"
    } else {
        // Timed wait
        auto duration = std::chrono::milliseconds(timeout);
        if (entry->cv.wait_for(lock, duration, [entry] { return entry->notified; })) {
            result = 0;  // "ok"
        } else {
            result = 2;  // "timed-out"
        }
    }

    // Remove from wait list
    {
        std::lock_guard<std::mutex> listLock(waitListMutex);
        auto& list = waitLists[address];
        list.erase(std::remove(list.begin(), list.end(), entry), list.end());
        if (list.empty()) {
            waitLists.erase(address);
        }
    }

    delete entry;
    return result;
}

// Atomics.notify(typedArray, index, count)
// Returns number of agents woken up
int64_t nova_atomics_notify(void* arrayPtr, int64_t index, int64_t count) {
    if (!arrayPtr) return 0;

    struct NovaTypedArray {
        void* buffer;
        uint8_t* data;
        int64_t byteOffset;
        int64_t byteLength;
        int64_t length;
        int64_t bytesPerElement;
        int64_t typeId;
    };

    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr->data || index < 0 || index >= arr->length) return 0;

    // Calculate address (works for both i32 and i64)
    uintptr_t address = reinterpret_cast<uintptr_t>(arr->data + index * arr->bytesPerElement);

    std::lock_guard<std::mutex> lock(waitListMutex);

    auto it = waitLists.find(address);
    if (it == waitLists.end()) {
        return 0;  // No waiters
    }

    auto& list = it->second;
    int64_t woken = 0;

    // Wake up 'count' waiters (or all if count is infinity/negative)
    for (auto& entry : list) {
        if (count >= 0 && woken >= count) break;

        {
            std::lock_guard<std::mutex> entryLock(entry->mutex);
            entry->notified = true;
        }
        entry->cv.notify_one();
        woken++;
    }

    return woken;
}

// Atomics.waitAsync - async version (simplified, returns result immediately for now)
// In full implementation, this would return a Promise
// Returns: 0 = "ok", 1 = "not-equal", 2 = "timed-out"
int64_t nova_atomics_waitAsync_i32(void* arrayPtr, int64_t index, int64_t value, int64_t timeout) {
    // Simplified: just check if values match
    if (!arrayPtr) return 1;

    struct NovaTypedArray {
        void* buffer;
        uint8_t* data;
        int64_t byteOffset;
        int64_t byteLength;
        int64_t length;
        int64_t bytesPerElement;
        int64_t typeId;
    };

    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr->data || index < 0 || index >= arr->length) return 1;

    std::atomic<int32_t>* atomicPtr = reinterpret_cast<std::atomic<int32_t>*>(
        arr->data + index * sizeof(int32_t));

    if (atomicPtr->load(std::memory_order_seq_cst) != static_cast<int32_t>(value)) {
        return 1;  // "not-equal"
    }

    return 0;  // "ok" - in full implementation, would create pending Promise
}

int64_t nova_atomics_waitAsync_i64(void* arrayPtr, int64_t index, int64_t value, int64_t timeout) {
    if (!arrayPtr) return 1;

    struct NovaTypedArray {
        void* buffer;
        uint8_t* data;
        int64_t byteOffset;
        int64_t byteLength;
        int64_t length;
        int64_t bytesPerElement;
        int64_t typeId;
    };

    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr->data || index < 0 || index >= arr->length) return 1;

    std::atomic<int64_t>* atomicPtr = reinterpret_cast<std::atomic<int64_t>*>(
        arr->data + index * sizeof(int64_t));

    if (atomicPtr->load(std::memory_order_seq_cst) != value) {
        return 1;  // "not-equal"
    }

    return 0;  // "ok"
}

} // extern "C"
