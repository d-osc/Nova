// Nova Runtime - ArrayBuffer and TypedArray Implementation
// Implements ArrayBuffer, DataView, and TypedArrays (Uint8Array, Int32Array, etc.)

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <string>

extern "C" {

// ============================================================================
// ArrayBuffer Structure
// ============================================================================
struct NovaArrayBuffer {
    uint8_t* data;      // Raw byte data
    int64_t byteLength; // Length in bytes
    bool detached;      // Whether buffer has been detached
};

// ============================================================================
// ArrayBuffer Creation
// ============================================================================

// new ArrayBuffer(length)
void* nova_arraybuffer_create(int64_t byteLength) {
    if (byteLength < 0) byteLength = 0;

    NovaArrayBuffer* buffer = new NovaArrayBuffer();
    buffer->byteLength = byteLength;
    buffer->detached = false;

    if (byteLength > 0) {
        buffer->data = static_cast<uint8_t*>(calloc(byteLength, 1)); // Zero-initialized
    } else {
        buffer->data = nullptr;
    }

    return buffer;
}

// ArrayBuffer.byteLength getter
int64_t nova_arraybuffer_byteLength(void* bufferPtr) {
    if (!bufferPtr) return 0;
    NovaArrayBuffer* buffer = static_cast<NovaArrayBuffer*>(bufferPtr);
    return buffer->detached ? 0 : buffer->byteLength;
}

// ArrayBuffer.prototype.slice(begin, end)
void* nova_arraybuffer_slice(void* bufferPtr, int64_t begin, int64_t end) {
    if (!bufferPtr) return nova_arraybuffer_create(0);
    NovaArrayBuffer* buffer = static_cast<NovaArrayBuffer*>(bufferPtr);

    if (buffer->detached) return nova_arraybuffer_create(0);

    int64_t len = buffer->byteLength;

    // Handle negative indices
    if (begin < 0) begin = std::max(len + begin, (int64_t)0);
    else begin = std::min(begin, len);

    if (end < 0) end = std::max(len + end, (int64_t)0);
    else end = std::min(end, len);

    int64_t newLen = std::max(end - begin, (int64_t)0);

    NovaArrayBuffer* newBuffer = static_cast<NovaArrayBuffer*>(nova_arraybuffer_create(newLen));

    if (newLen > 0 && buffer->data) {
        memcpy(newBuffer->data, buffer->data + begin, newLen);
    }

    return newBuffer;
}

// ArrayBuffer.isView(arg) - checks if arg is a TypedArray or DataView
int64_t nova_arraybuffer_isView([[maybe_unused]] void* arg) {
    // This would check if arg is a TypedArray or DataView
    // For now, return 0 (false) - will be enhanced when TypedArrays are added
    return 0;
}

// Free ArrayBuffer
void nova_arraybuffer_free(void* bufferPtr) {
    if (!bufferPtr) return;
    NovaArrayBuffer* buffer = static_cast<NovaArrayBuffer*>(bufferPtr);
    if (buffer->data) free(buffer->data);
    delete buffer;
}

// ============================================================================
// TypedArray Base Structure
// ============================================================================
struct NovaTypedArray {
    void* buffer;           // Underlying ArrayBuffer
    uint8_t* data;          // Direct pointer to data (for faster access)
    int64_t byteOffset;     // Offset into buffer
    int64_t byteLength;     // Length in bytes
    int64_t length;         // Number of elements
    int64_t bytesPerElement; // Size of each element (1, 2, 4, 8)
    int64_t typeId;         // Type identifier
};

// TypedArray type IDs
enum TypedArrayType {
    TYPED_INT8 = 1,
    TYPED_UINT8 = 2,
    TYPED_UINT8_CLAMPED = 3,
    TYPED_INT16 = 4,
    TYPED_UINT16 = 5,
    TYPED_INT32 = 6,
    TYPED_UINT32 = 7,
    TYPED_FLOAT32 = 8,
    TYPED_FLOAT64 = 9,
    TYPED_BIGINT64 = 10,
    TYPED_BIGUINT64 = 11
};

// Helper to create TypedArray from ArrayBuffer
static void* create_typed_array_from_buffer(void* bufferPtr, int64_t byteOffset,
                                            int64_t length, int64_t bytesPerElement, int64_t typeId) {
    NovaArrayBuffer* buffer = static_cast<NovaArrayBuffer*>(bufferPtr);

    NovaTypedArray* typed = new NovaTypedArray();
    typed->buffer = bufferPtr;
    typed->byteOffset = byteOffset;
    typed->bytesPerElement = bytesPerElement;
    typed->typeId = typeId;

    if (length < 0) {
        // Auto-calculate length from remaining buffer
        typed->byteLength = buffer->byteLength - byteOffset;
        typed->length = typed->byteLength / bytesPerElement;
    } else {
        typed->length = length;
        typed->byteLength = length * bytesPerElement;
    }

    typed->data = buffer->data + byteOffset;

    return typed;
}

// Helper to create TypedArray with new buffer
static void* create_typed_array_new(int64_t length, int64_t bytesPerElement, int64_t typeId) {
    int64_t byteLength = length * bytesPerElement;
    void* buffer = nova_arraybuffer_create(byteLength);
    return create_typed_array_from_buffer(buffer, 0, length, bytesPerElement, typeId);
}

// ============================================================================
// Int8Array
// ============================================================================

void* nova_int8array_create(int64_t length) {
    return create_typed_array_new(length, 1, TYPED_INT8);
}

void* nova_int8array_from_buffer(void* buffer, int64_t byteOffset, int64_t length) {
    return create_typed_array_from_buffer(buffer, byteOffset, length, 1, TYPED_INT8);
}

int64_t nova_int8array_get(void* arrayPtr, int64_t index) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || index < 0 || index >= arr->length) return 0;
    return static_cast<int8_t*>(static_cast<void*>(arr->data))[index];
}

void nova_int8array_set(void* arrayPtr, int64_t index, int64_t value) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || index < 0 || index >= arr->length) return;
    static_cast<int8_t*>(static_cast<void*>(arr->data))[index] = static_cast<int8_t>(value);
}

// ============================================================================
// Uint8Array
// ============================================================================

void* nova_uint8array_create(int64_t length) {
    return create_typed_array_new(length, 1, TYPED_UINT8);
}

void* nova_uint8array_from_buffer(void* buffer, int64_t byteOffset, int64_t length) {
    return create_typed_array_from_buffer(buffer, byteOffset, length, 1, TYPED_UINT8);
}

int64_t nova_uint8array_get(void* arrayPtr, int64_t index) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || index < 0 || index >= arr->length) return 0;
    return arr->data[index];
}

void nova_uint8array_set(void* arrayPtr, int64_t index, int64_t value) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || index < 0 || index >= arr->length) return;
    arr->data[index] = static_cast<uint8_t>(value);
}

// ============================================================================
// Uint8ClampedArray
// ============================================================================

void* nova_uint8clampedarray_create(int64_t length) {
    return create_typed_array_new(length, 1, TYPED_UINT8_CLAMPED);
}

void* nova_uint8clampedarray_from_buffer(void* buffer, int64_t byteOffset, int64_t length) {
    return create_typed_array_from_buffer(buffer, byteOffset, length, 1, TYPED_UINT8_CLAMPED);
}

int64_t nova_uint8clampedarray_get(void* arrayPtr, int64_t index) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || index < 0 || index >= arr->length) return 0;
    return arr->data[index];
}

void nova_uint8clampedarray_set(void* arrayPtr, int64_t index, int64_t value) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || index < 0 || index >= arr->length) return;
    // Clamp value to 0-255
    if (value < 0) value = 0;
    if (value > 255) value = 255;
    arr->data[index] = static_cast<uint8_t>(value);
}

// ============================================================================
// Int16Array
// ============================================================================

void* nova_int16array_create(int64_t length) {
    return create_typed_array_new(length, 2, TYPED_INT16);
}

void* nova_int16array_from_buffer(void* buffer, int64_t byteOffset, int64_t length) {
    return create_typed_array_from_buffer(buffer, byteOffset, length, 2, TYPED_INT16);
}

int64_t nova_int16array_get(void* arrayPtr, int64_t index) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || index < 0 || index >= arr->length) return 0;
    return reinterpret_cast<int16_t*>(arr->data)[index];
}

void nova_int16array_set(void* arrayPtr, int64_t index, int64_t value) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || index < 0 || index >= arr->length) return;
    reinterpret_cast<int16_t*>(arr->data)[index] = static_cast<int16_t>(value);
}

// ============================================================================
// Uint16Array
// ============================================================================

void* nova_uint16array_create(int64_t length) {
    return create_typed_array_new(length, 2, TYPED_UINT16);
}

void* nova_uint16array_from_buffer(void* buffer, int64_t byteOffset, int64_t length) {
    return create_typed_array_from_buffer(buffer, byteOffset, length, 2, TYPED_UINT16);
}

int64_t nova_uint16array_get(void* arrayPtr, int64_t index) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || index < 0 || index >= arr->length) return 0;
    return reinterpret_cast<uint16_t*>(arr->data)[index];
}

void nova_uint16array_set(void* arrayPtr, int64_t index, int64_t value) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || index < 0 || index >= arr->length) return;
    reinterpret_cast<uint16_t*>(arr->data)[index] = static_cast<uint16_t>(value);
}

// ============================================================================
// Int32Array
// ============================================================================

void* nova_int32array_create(int64_t length) {
    return create_typed_array_new(length, 4, TYPED_INT32);
}

void* nova_int32array_from_buffer(void* buffer, int64_t byteOffset, int64_t length) {
    return create_typed_array_from_buffer(buffer, byteOffset, length, 4, TYPED_INT32);
}

int64_t nova_int32array_get(void* arrayPtr, int64_t index) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || index < 0 || index >= arr->length) return 0;
    return reinterpret_cast<int32_t*>(arr->data)[index];
}

void nova_int32array_set(void* arrayPtr, int64_t index, int64_t value) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || index < 0 || index >= arr->length) return;
    reinterpret_cast<int32_t*>(arr->data)[index] = static_cast<int32_t>(value);
}

// ============================================================================
// Uint32Array
// ============================================================================

void* nova_uint32array_create(int64_t length) {
    return create_typed_array_new(length, 4, TYPED_UINT32);
}

void* nova_uint32array_from_buffer(void* buffer, int64_t byteOffset, int64_t length) {
    return create_typed_array_from_buffer(buffer, byteOffset, length, 4, TYPED_UINT32);
}

int64_t nova_uint32array_get(void* arrayPtr, int64_t index) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || index < 0 || index >= arr->length) return 0;
    return reinterpret_cast<uint32_t*>(arr->data)[index];
}

void nova_uint32array_set(void* arrayPtr, int64_t index, int64_t value) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || index < 0 || index >= arr->length) return;
    reinterpret_cast<uint32_t*>(arr->data)[index] = static_cast<uint32_t>(value);
}

// ============================================================================
// Float32Array
// ============================================================================

void* nova_float32array_create(int64_t length) {
    return create_typed_array_new(length, 4, TYPED_FLOAT32);
}

void* nova_float32array_from_buffer(void* buffer, int64_t byteOffset, int64_t length) {
    return create_typed_array_from_buffer(buffer, byteOffset, length, 4, TYPED_FLOAT32);
}

double nova_float32array_get(void* arrayPtr, int64_t index) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || index < 0 || index >= arr->length) return 0.0;
    return static_cast<double>(reinterpret_cast<float*>(arr->data)[index]);
}

void nova_float32array_set(void* arrayPtr, int64_t index, double value) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || index < 0 || index >= arr->length) return;
    reinterpret_cast<float*>(arr->data)[index] = static_cast<float>(value);
}

// ============================================================================
// Float64Array
// ============================================================================

void* nova_float64array_create(int64_t length) {
    return create_typed_array_new(length, 8, TYPED_FLOAT64);
}

void* nova_float64array_from_buffer(void* buffer, int64_t byteOffset, int64_t length) {
    return create_typed_array_from_buffer(buffer, byteOffset, length, 8, TYPED_FLOAT64);
}

double nova_float64array_get(void* arrayPtr, int64_t index) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || index < 0 || index >= arr->length) return 0.0;
    return reinterpret_cast<double*>(arr->data)[index];
}

void nova_float64array_set(void* arrayPtr, int64_t index, double value) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || index < 0 || index >= arr->length) return;
    reinterpret_cast<double*>(arr->data)[index] = value;
}

// ============================================================================
// BigInt64Array
// ============================================================================

void* nova_bigint64array_create(int64_t length) {
    return create_typed_array_new(length, 8, TYPED_BIGINT64);
}

void* nova_bigint64array_from_buffer(void* buffer, int64_t byteOffset, int64_t length) {
    return create_typed_array_from_buffer(buffer, byteOffset, length, 8, TYPED_BIGINT64);
}

int64_t nova_bigint64array_get(void* arrayPtr, int64_t index) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || index < 0 || index >= arr->length) return 0;
    return reinterpret_cast<int64_t*>(arr->data)[index];
}

void nova_bigint64array_set(void* arrayPtr, int64_t index, int64_t value) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || index < 0 || index >= arr->length) return;
    reinterpret_cast<int64_t*>(arr->data)[index] = value;
}

// ============================================================================
// BigUint64Array
// ============================================================================

void* nova_biguint64array_create(int64_t length) {
    return create_typed_array_new(length, 8, TYPED_BIGUINT64);
}

void* nova_biguint64array_from_buffer(void* buffer, int64_t byteOffset, int64_t length) {
    return create_typed_array_from_buffer(buffer, byteOffset, length, 8, TYPED_BIGUINT64);
}

uint64_t nova_biguint64array_get(void* arrayPtr, int64_t index) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || index < 0 || index >= arr->length) return 0;
    return reinterpret_cast<uint64_t*>(arr->data)[index];
}

void nova_biguint64array_set(void* arrayPtr, int64_t index, uint64_t value) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || index < 0 || index >= arr->length) return;
    reinterpret_cast<uint64_t*>(arr->data)[index] = value;
}

// ============================================================================
// Common TypedArray Properties
// ============================================================================

int64_t nova_typedarray_length(void* arrayPtr) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    return arr ? arr->length : 0;
}

int64_t nova_typedarray_byteLength(void* arrayPtr) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    return arr ? arr->byteLength : 0;
}

int64_t nova_typedarray_byteOffset(void* arrayPtr) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    return arr ? arr->byteOffset : 0;
}

void* nova_typedarray_buffer(void* arrayPtr) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    return arr ? arr->buffer : nullptr;
}

int64_t nova_typedarray_BYTES_PER_ELEMENT(void* arrayPtr) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    return arr ? arr->bytesPerElement : 0;
}

// ============================================================================
// TypedArray Methods
// ============================================================================

// TypedArray.prototype.set(array, offset)
void nova_typedarray_set_array(void* destPtr, void* srcPtr, int64_t offset) {
    NovaTypedArray* dest = static_cast<NovaTypedArray*>(destPtr);
    NovaTypedArray* src = static_cast<NovaTypedArray*>(srcPtr);

    if (!dest || !src || offset < 0) return;
    if (offset + src->length > dest->length) return;

    // Copy bytes (simplified - same type assumed)
    memcpy(dest->data + offset * dest->bytesPerElement,
           src->data,
           src->length * src->bytesPerElement);
}

// TypedArray.prototype.subarray(begin, end)
void* nova_typedarray_subarray(void* arrayPtr, int64_t begin, int64_t end) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr) return nullptr;

    int64_t len = arr->length;

    // Handle negative indices
    if (begin < 0) begin = std::max(len + begin, (int64_t)0);
    else begin = std::min(begin, len);

    if (end < 0) end = std::max(len + end, (int64_t)0);
    else end = std::min(end, len);

    int64_t newLen = std::max(end - begin, (int64_t)0);
    int64_t newByteOffset = arr->byteOffset + begin * arr->bytesPerElement;

    return create_typed_array_from_buffer(arr->buffer, newByteOffset, newLen,
                                          arr->bytesPerElement, arr->typeId);
}

// TypedArray.prototype.slice(begin, end)
void* nova_typedarray_slice(void* arrayPtr, int64_t begin, int64_t end) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr) return nullptr;

    int64_t len = arr->length;

    // Handle negative indices
    if (begin < 0) begin = std::max(len + begin, (int64_t)0);
    else begin = std::min(begin, len);

    if (end < 0) end = std::max(len + end, (int64_t)0);
    else end = std::min(end, len);

    int64_t newLen = std::max(end - begin, (int64_t)0);

    // Create new TypedArray with new buffer
    NovaTypedArray* result = static_cast<NovaTypedArray*>(
        create_typed_array_new(newLen, arr->bytesPerElement, arr->typeId));

    if (newLen > 0) {
        memcpy(result->data, arr->data + begin * arr->bytesPerElement,
               newLen * arr->bytesPerElement);
    }

    return result;
}

// TypedArray.prototype.fill(value, start, end)
void* nova_typedarray_fill(void* arrayPtr, int64_t value, int64_t start, int64_t end) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr) return arrayPtr;

    int64_t len = arr->length;

    if (start < 0) start = std::max(len + start, (int64_t)0);
    else start = std::min(start, len);

    if (end < 0) end = std::max(len + end, (int64_t)0);
    else end = std::min(end, len);

    // Fill based on type
    switch (arr->typeId) {
        case TYPED_INT8:
        case TYPED_UINT8:
        case TYPED_UINT8_CLAMPED:
            for (int64_t i = start; i < end; i++) {
                arr->data[i] = static_cast<uint8_t>(value);
            }
            break;
        case TYPED_INT16:
        case TYPED_UINT16:
            for (int64_t i = start; i < end; i++) {
                reinterpret_cast<uint16_t*>(arr->data)[i] = static_cast<uint16_t>(value);
            }
            break;
        case TYPED_INT32:
        case TYPED_UINT32:
            for (int64_t i = start; i < end; i++) {
                reinterpret_cast<uint32_t*>(arr->data)[i] = static_cast<uint32_t>(value);
            }
            break;
        case TYPED_BIGINT64:
        case TYPED_BIGUINT64:
            for (int64_t i = start; i < end; i++) {
                reinterpret_cast<int64_t*>(arr->data)[i] = value;
            }
            break;
    }

    return arrayPtr;
}

// TypedArray.prototype.copyWithin(target, start, end)
void* nova_typedarray_copyWithin(void* arrayPtr, int64_t target, int64_t start, int64_t end) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr) return arrayPtr;

    int64_t len = arr->length;

    if (target < 0) target = std::max(len + target, (int64_t)0);
    else target = std::min(target, len);

    if (start < 0) start = std::max(len + start, (int64_t)0);
    else start = std::min(start, len);

    if (end < 0) end = std::max(len + end, (int64_t)0);
    else end = std::min(end, len);

    int64_t count = std::min(end - start, len - target);
    if (count <= 0) return arrayPtr;

    memmove(arr->data + target * arr->bytesPerElement,
            arr->data + start * arr->bytesPerElement,
            count * arr->bytesPerElement);

    return arrayPtr;
}

// TypedArray.prototype.reverse()
void* nova_typedarray_reverse(void* arrayPtr) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || arr->length <= 1) return arrayPtr;

    int64_t bpe = arr->bytesPerElement;
    uint8_t* temp = static_cast<uint8_t*>(malloc(bpe));

    for (int64_t i = 0; i < arr->length / 2; i++) {
        int64_t j = arr->length - 1 - i;
        memcpy(temp, arr->data + i * bpe, bpe);
        memcpy(arr->data + i * bpe, arr->data + j * bpe, bpe);
        memcpy(arr->data + j * bpe, temp, bpe);
    }

    free(temp);
    return arrayPtr;
}

// TypedArray.prototype.indexOf(searchElement, fromIndex)
int64_t nova_typedarray_indexOf(void* arrayPtr, int64_t searchElement, int64_t fromIndex) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr) return -1;

    if (fromIndex < 0) fromIndex = std::max(arr->length + fromIndex, (int64_t)0);

    for (int64_t i = fromIndex; i < arr->length; i++) {
        int64_t val = 0;
        switch (arr->typeId) {
            case TYPED_INT8: val = reinterpret_cast<int8_t*>(arr->data)[i]; break;
            case TYPED_UINT8:
            case TYPED_UINT8_CLAMPED: val = arr->data[i]; break;
            case TYPED_INT16: val = reinterpret_cast<int16_t*>(arr->data)[i]; break;
            case TYPED_UINT16: val = reinterpret_cast<uint16_t*>(arr->data)[i]; break;
            case TYPED_INT32: val = reinterpret_cast<int32_t*>(arr->data)[i]; break;
            case TYPED_UINT32: val = reinterpret_cast<uint32_t*>(arr->data)[i]; break;
            case TYPED_BIGINT64: val = reinterpret_cast<int64_t*>(arr->data)[i]; break;
            case TYPED_BIGUINT64: val = reinterpret_cast<int64_t*>(arr->data)[i]; break;
        }
        if (val == searchElement) return i;
    }

    return -1;
}

// TypedArray.prototype.includes(searchElement, fromIndex)
int64_t nova_typedarray_includes(void* arrayPtr, int64_t searchElement, int64_t fromIndex) {
    return nova_typedarray_indexOf(arrayPtr, searchElement, fromIndex) >= 0 ? 1 : 0;
}

// TypedArray.prototype.at(index)
int64_t nova_typedarray_at(void* arrayPtr, int64_t index) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr) return 0;

    int64_t len = arr->length;
    // Handle negative indices
    if (index < 0) index = len + index;
    if (index < 0 || index >= len) return 0;  // undefined -> 0

    // Return based on type
    switch (arr->typeId) {
        case TYPED_INT8: return static_cast<int8_t*>(static_cast<void*>(arr->data))[index];
        case TYPED_UINT8:
        case TYPED_UINT8_CLAMPED: return arr->data[index];
        case TYPED_INT16: return reinterpret_cast<int16_t*>(arr->data)[index];
        case TYPED_UINT16: return reinterpret_cast<uint16_t*>(arr->data)[index];
        case TYPED_INT32: return reinterpret_cast<int32_t*>(arr->data)[index];
        case TYPED_UINT32: return reinterpret_cast<uint32_t*>(arr->data)[index];
        case TYPED_BIGINT64:
        case TYPED_BIGUINT64: return reinterpret_cast<int64_t*>(arr->data)[index];
        default: return arr->data[index];
    }
}

// TypedArray.prototype.lastIndexOf(searchElement, fromIndex)
int64_t nova_typedarray_lastIndexOf(void* arrayPtr, int64_t searchElement, int64_t fromIndex) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || arr->length == 0) return -1;

    int64_t len = arr->length;
    int64_t start = (fromIndex >= len) ? len - 1 : fromIndex;
    if (fromIndex < 0) start = len + fromIndex;
    if (start < 0) return -1;

    // Search backwards based on type
    for (int64_t i = start; i >= 0; i--) {
        int64_t element = 0;
        switch (arr->typeId) {
            case TYPED_INT8: element = static_cast<int8_t*>(static_cast<void*>(arr->data))[i]; break;
            case TYPED_UINT8:
            case TYPED_UINT8_CLAMPED: element = arr->data[i]; break;
            case TYPED_INT16: element = reinterpret_cast<int16_t*>(arr->data)[i]; break;
            case TYPED_UINT16: element = reinterpret_cast<uint16_t*>(arr->data)[i]; break;
            case TYPED_INT32: element = reinterpret_cast<int32_t*>(arr->data)[i]; break;
            case TYPED_UINT32: element = reinterpret_cast<uint32_t*>(arr->data)[i]; break;
            case TYPED_BIGINT64:
            case TYPED_BIGUINT64: element = reinterpret_cast<int64_t*>(arr->data)[i]; break;
        }
        if (element == searchElement) return i;
    }
    return -1;
}

// TypedArray.prototype.sort() - sorts in place numerically
void* nova_typedarray_sort(void* arrayPtr) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || arr->length <= 1) return arrayPtr;

    int64_t len = arr->length;

    // Simple bubble sort for now (can optimize later)
    switch (arr->typeId) {
        case TYPED_INT8:
            std::sort(static_cast<int8_t*>(static_cast<void*>(arr->data)),
                      static_cast<int8_t*>(static_cast<void*>(arr->data)) + len);
            break;
        case TYPED_UINT8:
        case TYPED_UINT8_CLAMPED:
            std::sort(arr->data, arr->data + len);
            break;
        case TYPED_INT16:
            std::sort(reinterpret_cast<int16_t*>(arr->data),
                      reinterpret_cast<int16_t*>(arr->data) + len);
            break;
        case TYPED_UINT16:
            std::sort(reinterpret_cast<uint16_t*>(arr->data),
                      reinterpret_cast<uint16_t*>(arr->data) + len);
            break;
        case TYPED_INT32:
            std::sort(reinterpret_cast<int32_t*>(arr->data),
                      reinterpret_cast<int32_t*>(arr->data) + len);
            break;
        case TYPED_UINT32:
            std::sort(reinterpret_cast<uint32_t*>(arr->data),
                      reinterpret_cast<uint32_t*>(arr->data) + len);
            break;
        case TYPED_FLOAT32:
            std::sort(reinterpret_cast<float*>(arr->data),
                      reinterpret_cast<float*>(arr->data) + len);
            break;
        case TYPED_FLOAT64:
            std::sort(reinterpret_cast<double*>(arr->data),
                      reinterpret_cast<double*>(arr->data) + len);
            break;
        case TYPED_BIGINT64:
            std::sort(reinterpret_cast<int64_t*>(arr->data),
                      reinterpret_cast<int64_t*>(arr->data) + len);
            break;
        case TYPED_BIGUINT64:
            std::sort(reinterpret_cast<uint64_t*>(arr->data),
                      reinterpret_cast<uint64_t*>(arr->data) + len);
            break;
    }
    return arrayPtr;
}

// TypedArray.prototype.toSorted() - returns new sorted array
void* nova_typedarray_toSorted(void* arrayPtr) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr) return nova_uint8array_create(0);

    // Create a copy
    void* copy = nova_typedarray_slice(arrayPtr, 0, arr->length);
    // Sort the copy
    return nova_typedarray_sort(copy);
}

// TypedArray.prototype.toReversed() - returns new reversed array
void* nova_typedarray_toReversed(void* arrayPtr) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr) return nova_uint8array_create(0);

    // Create a copy
    void* copy = nova_typedarray_slice(arrayPtr, 0, arr->length);
    // Reverse the copy
    return nova_typedarray_reverse(copy);
}

// TypedArray.prototype.join(separator)
void* nova_typedarray_join(void* arrayPtr, const char* separator) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || arr->length == 0) {
        char* result = new char[1];
        result[0] = '\0';
        return result;
    }

    if (!separator) separator = ",";

    // Calculate buffer size
    std::string result;
    for (int64_t i = 0; i < arr->length; i++) {
        if (i > 0) result += separator;

        int64_t element = 0;
        switch (arr->typeId) {
            case TYPED_INT8: element = static_cast<int8_t*>(static_cast<void*>(arr->data))[i]; break;
            case TYPED_UINT8:
            case TYPED_UINT8_CLAMPED: element = arr->data[i]; break;
            case TYPED_INT16: element = reinterpret_cast<int16_t*>(arr->data)[i]; break;
            case TYPED_UINT16: element = reinterpret_cast<uint16_t*>(arr->data)[i]; break;
            case TYPED_INT32: element = reinterpret_cast<int32_t*>(arr->data)[i]; break;
            case TYPED_UINT32: element = reinterpret_cast<uint32_t*>(arr->data)[i]; break;
            case TYPED_BIGINT64:
            case TYPED_BIGUINT64: element = reinterpret_cast<int64_t*>(arr->data)[i]; break;
        }
        result += std::to_string(element);
    }

    char* strResult = new char[result.length() + 1];
    strcpy(strResult, result.c_str());
    return strResult;
}

// TypedArray.prototype.toString() - returns comma-separated string
void* nova_typedarray_toString(void* arrayPtr) {
    // toString is just join with default separator ","
    return nova_typedarray_join(arrayPtr, ",");
}

// TypedArray.prototype.toLocaleString() - returns locale-aware comma-separated string
// Note: For simplicity, this uses standard number formatting
// Full locale support would require ICU integration
void* nova_typedarray_toLocaleString(void* arrayPtr) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || arr->length == 0) {
        char* result = new char[1];
        result[0] = '\0';
        return result;
    }

    std::string result;
    for (int64_t i = 0; i < arr->length; i++) {
        if (i > 0) result += ",";

        // Handle all types inline
        switch (arr->typeId) {
            case TYPED_FLOAT32: {
                float val = reinterpret_cast<float*>(arr->data)[i];
                char buf[64];
                snprintf(buf, sizeof(buf), "%g", static_cast<double>(val));
                result += buf;
                break;
            }
            case TYPED_FLOAT64: {
                double val = reinterpret_cast<double*>(arr->data)[i];
                char buf[64];
                snprintf(buf, sizeof(buf), "%g", val);
                result += buf;
                break;
            }
            case TYPED_INT8:
                result += std::to_string(static_cast<int8_t*>(static_cast<void*>(arr->data))[i]);
                break;
            case TYPED_UINT8:
            case TYPED_UINT8_CLAMPED:
                result += std::to_string(arr->data[i]);
                break;
            case TYPED_INT16:
                result += std::to_string(reinterpret_cast<int16_t*>(arr->data)[i]);
                break;
            case TYPED_UINT16:
                result += std::to_string(reinterpret_cast<uint16_t*>(arr->data)[i]);
                break;
            case TYPED_INT32:
                result += std::to_string(reinterpret_cast<int32_t*>(arr->data)[i]);
                break;
            case TYPED_UINT32:
                result += std::to_string(reinterpret_cast<uint32_t*>(arr->data)[i]);
                break;
            case TYPED_BIGINT64:
                result += std::to_string(reinterpret_cast<int64_t*>(arr->data)[i]);
                break;
            case TYPED_BIGUINT64:
                result += std::to_string(reinterpret_cast<uint64_t*>(arr->data)[i]);
                break;
            default:
                result += std::to_string(arr->data[i]);
                break;
        }
    }

    char* strResult = new char[result.length() + 1];
    strcpy(strResult, result.c_str());
    return strResult;
}

// TypedArray.prototype.with(index, value) - returns copy with element replaced (ES2023)
void* nova_int8array_with(void* arrayPtr, int64_t index, int64_t value) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr) return nullptr;

    // Handle negative index
    if (index < 0) index = arr->length + index;
    if (index < 0 || index >= arr->length) return nullptr; // RangeError in JS

    // Create a copy
    void* copy = nova_typedarray_slice(arrayPtr, 0, arr->length);
    nova_int8array_set(copy, index, value);
    return copy;
}

void* nova_uint8array_with(void* arrayPtr, int64_t index, int64_t value) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr) return nullptr;

    if (index < 0) index = arr->length + index;
    if (index < 0 || index >= arr->length) return nullptr;

    void* copy = nova_typedarray_slice(arrayPtr, 0, arr->length);
    nova_uint8array_set(copy, index, value);
    return copy;
}

void* nova_uint8clampedarray_with(void* arrayPtr, int64_t index, int64_t value) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr) return nullptr;

    if (index < 0) index = arr->length + index;
    if (index < 0 || index >= arr->length) return nullptr;

    void* copy = nova_typedarray_slice(arrayPtr, 0, arr->length);
    nova_uint8clampedarray_set(copy, index, value);
    return copy;
}

void* nova_int16array_with(void* arrayPtr, int64_t index, int64_t value) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr) return nullptr;

    if (index < 0) index = arr->length + index;
    if (index < 0 || index >= arr->length) return nullptr;

    void* copy = nova_typedarray_slice(arrayPtr, 0, arr->length);
    nova_int16array_set(copy, index, value);
    return copy;
}

void* nova_uint16array_with(void* arrayPtr, int64_t index, int64_t value) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr) return nullptr;

    if (index < 0) index = arr->length + index;
    if (index < 0 || index >= arr->length) return nullptr;

    void* copy = nova_typedarray_slice(arrayPtr, 0, arr->length);
    nova_uint16array_set(copy, index, value);
    return copy;
}

void* nova_int32array_with(void* arrayPtr, int64_t index, int64_t value) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr) return nullptr;

    if (index < 0) index = arr->length + index;
    if (index < 0 || index >= arr->length) return nullptr;

    void* copy = nova_typedarray_slice(arrayPtr, 0, arr->length);
    nova_int32array_set(copy, index, value);
    return copy;
}

void* nova_uint32array_with(void* arrayPtr, int64_t index, int64_t value) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr) return nullptr;

    if (index < 0) index = arr->length + index;
    if (index < 0 || index >= arr->length) return nullptr;

    void* copy = nova_typedarray_slice(arrayPtr, 0, arr->length);
    nova_uint32array_set(copy, index, value);
    return copy;
}

void* nova_float32array_with(void* arrayPtr, int64_t index, double value) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr) return nullptr;

    if (index < 0) index = arr->length + index;
    if (index < 0 || index >= arr->length) return nullptr;

    void* copy = nova_typedarray_slice(arrayPtr, 0, arr->length);
    nova_float32array_set(copy, index, value);
    return copy;
}

void* nova_float64array_with(void* arrayPtr, int64_t index, double value) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr) return nullptr;

    if (index < 0) index = arr->length + index;
    if (index < 0 || index >= arr->length) return nullptr;

    void* copy = nova_typedarray_slice(arrayPtr, 0, arr->length);
    nova_float64array_set(copy, index, value);
    return copy;
}

void* nova_bigint64array_with(void* arrayPtr, int64_t index, int64_t value) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr) return nullptr;

    if (index < 0) index = arr->length + index;
    if (index < 0 || index >= arr->length) return nullptr;

    void* copy = nova_typedarray_slice(arrayPtr, 0, arr->length);
    nova_bigint64array_set(copy, index, value);
    return copy;
}

void* nova_biguint64array_with(void* arrayPtr, int64_t index, int64_t value) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr) return nullptr;

    if (index < 0) index = arr->length + index;
    if (index < 0 || index >= arr->length) return nullptr;

    void* copy = nova_typedarray_slice(arrayPtr, 0, arr->length);
    nova_biguint64array_set(copy, index, value);
    return copy;
}

// ============================================================================
// DataView
// ============================================================================
struct NovaDataView {
    void* buffer;           // Underlying ArrayBuffer
    uint8_t* data;          // Direct pointer to data
    int64_t byteOffset;     // Offset into buffer
    int64_t byteLength;     // Length in bytes
};

// new DataView(buffer, byteOffset, byteLength)
void* nova_dataview_create(void* bufferPtr, int64_t byteOffset, int64_t byteLength) {
    NovaArrayBuffer* buffer = static_cast<NovaArrayBuffer*>(bufferPtr);
    if (!buffer) return nullptr;

    NovaDataView* view = new NovaDataView();
    view->buffer = bufferPtr;
    view->byteOffset = byteOffset;

    if (byteLength < 0) {
        view->byteLength = buffer->byteLength - byteOffset;
    } else {
        view->byteLength = byteLength;
    }

    view->data = buffer->data + byteOffset;

    return view;
}

int64_t nova_dataview_byteLength(void* viewPtr) {
    NovaDataView* view = static_cast<NovaDataView*>(viewPtr);
    return view ? view->byteLength : 0;
}

int64_t nova_dataview_byteOffset(void* viewPtr) {
    NovaDataView* view = static_cast<NovaDataView*>(viewPtr);
    return view ? view->byteOffset : 0;
}

void* nova_dataview_buffer(void* viewPtr) {
    NovaDataView* view = static_cast<NovaDataView*>(viewPtr);
    return view ? view->buffer : nullptr;
}

// DataView getters (littleEndian = 1 for little endian, 0 for big endian)
int64_t nova_dataview_getInt8(void* viewPtr, int64_t byteOffset) {
    NovaDataView* view = static_cast<NovaDataView*>(viewPtr);
    if (!view || byteOffset < 0 || byteOffset >= view->byteLength) return 0;
    return static_cast<int8_t>(view->data[byteOffset]);
}

int64_t nova_dataview_getUint8(void* viewPtr, int64_t byteOffset) {
    NovaDataView* view = static_cast<NovaDataView*>(viewPtr);
    if (!view || byteOffset < 0 || byteOffset >= view->byteLength) return 0;
    return view->data[byteOffset];
}

int64_t nova_dataview_getInt16(void* viewPtr, int64_t byteOffset, int64_t littleEndian) {
    NovaDataView* view = static_cast<NovaDataView*>(viewPtr);
    if (!view || byteOffset < 0 || byteOffset + 2 > view->byteLength) return 0;

    uint8_t* p = view->data + byteOffset;
    if (littleEndian) {
        return static_cast<int16_t>(p[0] | (p[1] << 8));
    } else {
        return static_cast<int16_t>((p[0] << 8) | p[1]);
    }
}

int64_t nova_dataview_getUint16(void* viewPtr, int64_t byteOffset, int64_t littleEndian) {
    NovaDataView* view = static_cast<NovaDataView*>(viewPtr);
    if (!view || byteOffset < 0 || byteOffset + 2 > view->byteLength) return 0;

    uint8_t* p = view->data + byteOffset;
    if (littleEndian) {
        return static_cast<uint16_t>(p[0] | (p[1] << 8));
    } else {
        return static_cast<uint16_t>((p[0] << 8) | p[1]);
    }
}

int64_t nova_dataview_getInt32(void* viewPtr, int64_t byteOffset, int64_t littleEndian) {
    NovaDataView* view = static_cast<NovaDataView*>(viewPtr);
    if (!view || byteOffset < 0 || byteOffset + 4 > view->byteLength) return 0;

    uint8_t* p = view->data + byteOffset;
    if (littleEndian) {
        return static_cast<int32_t>(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
    } else {
        return static_cast<int32_t>((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
    }
}

int64_t nova_dataview_getUint32(void* viewPtr, int64_t byteOffset, int64_t littleEndian) {
    NovaDataView* view = static_cast<NovaDataView*>(viewPtr);
    if (!view || byteOffset < 0 || byteOffset + 4 > view->byteLength) return 0;

    uint8_t* p = view->data + byteOffset;
    if (littleEndian) {
        return static_cast<uint32_t>(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
    } else {
        return static_cast<uint32_t>((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
    }
}

double nova_dataview_getFloat32(void* viewPtr, int64_t byteOffset, int64_t littleEndian) {
    NovaDataView* view = static_cast<NovaDataView*>(viewPtr);
    if (!view || byteOffset < 0 || byteOffset + 4 > view->byteLength) return 0.0;

    uint32_t bits = static_cast<uint32_t>(nova_dataview_getUint32(viewPtr, byteOffset, littleEndian));
    float result;
    memcpy(&result, &bits, 4);
    return result;
}

double nova_dataview_getFloat64(void* viewPtr, int64_t byteOffset, int64_t littleEndian) {
    NovaDataView* view = static_cast<NovaDataView*>(viewPtr);
    if (!view || byteOffset < 0 || byteOffset + 8 > view->byteLength) return 0.0;

    uint8_t* p = view->data + byteOffset;
    uint64_t bits;
    if (littleEndian) {
        bits = static_cast<uint64_t>(p[0]) | (static_cast<uint64_t>(p[1]) << 8) |
               (static_cast<uint64_t>(p[2]) << 16) | (static_cast<uint64_t>(p[3]) << 24) |
               (static_cast<uint64_t>(p[4]) << 32) | (static_cast<uint64_t>(p[5]) << 40) |
               (static_cast<uint64_t>(p[6]) << 48) | (static_cast<uint64_t>(p[7]) << 56);
    } else {
        bits = (static_cast<uint64_t>(p[0]) << 56) | (static_cast<uint64_t>(p[1]) << 48) |
               (static_cast<uint64_t>(p[2]) << 40) | (static_cast<uint64_t>(p[3]) << 32) |
               (static_cast<uint64_t>(p[4]) << 24) | (static_cast<uint64_t>(p[5]) << 16) |
               (static_cast<uint64_t>(p[6]) << 8) | static_cast<uint64_t>(p[7]);
    }
    double result;
    memcpy(&result, &bits, 8);
    return result;
}

// DataView setters
void nova_dataview_setInt8(void* viewPtr, int64_t byteOffset, int64_t value) {
    NovaDataView* view = static_cast<NovaDataView*>(viewPtr);
    if (!view || byteOffset < 0 || byteOffset >= view->byteLength) return;
    view->data[byteOffset] = static_cast<int8_t>(value);
}

void nova_dataview_setUint8(void* viewPtr, int64_t byteOffset, int64_t value) {
    NovaDataView* view = static_cast<NovaDataView*>(viewPtr);
    if (!view || byteOffset < 0 || byteOffset >= view->byteLength) return;
    view->data[byteOffset] = static_cast<uint8_t>(value);
}

void nova_dataview_setInt16(void* viewPtr, int64_t byteOffset, int64_t value, int64_t littleEndian) {
    NovaDataView* view = static_cast<NovaDataView*>(viewPtr);
    if (!view || byteOffset < 0 || byteOffset + 2 > view->byteLength) return;

    uint8_t* p = view->data + byteOffset;
    if (littleEndian) {
        p[0] = value & 0xFF;
        p[1] = (value >> 8) & 0xFF;
    } else {
        p[0] = (value >> 8) & 0xFF;
        p[1] = value & 0xFF;
    }
}

void nova_dataview_setUint16(void* viewPtr, int64_t byteOffset, int64_t value, int64_t littleEndian) {
    nova_dataview_setInt16(viewPtr, byteOffset, value, littleEndian);
}

void nova_dataview_setInt32(void* viewPtr, int64_t byteOffset, int64_t value, int64_t littleEndian) {
    NovaDataView* view = static_cast<NovaDataView*>(viewPtr);
    if (!view || byteOffset < 0 || byteOffset + 4 > view->byteLength) return;

    uint8_t* p = view->data + byteOffset;
    if (littleEndian) {
        p[0] = value & 0xFF;
        p[1] = (value >> 8) & 0xFF;
        p[2] = (value >> 16) & 0xFF;
        p[3] = (value >> 24) & 0xFF;
    } else {
        p[0] = (value >> 24) & 0xFF;
        p[1] = (value >> 16) & 0xFF;
        p[2] = (value >> 8) & 0xFF;
        p[3] = value & 0xFF;
    }
}

void nova_dataview_setUint32(void* viewPtr, int64_t byteOffset, int64_t value, int64_t littleEndian) {
    nova_dataview_setInt32(viewPtr, byteOffset, value, littleEndian);
}

void nova_dataview_setFloat32(void* viewPtr, int64_t byteOffset, double value, int64_t littleEndian) {
    float f = static_cast<float>(value);
    uint32_t bits;
    memcpy(&bits, &f, 4);
    nova_dataview_setUint32(viewPtr, byteOffset, bits, littleEndian);
}

void nova_dataview_setFloat64(void* viewPtr, int64_t byteOffset, double value, int64_t littleEndian) {
    NovaDataView* view = static_cast<NovaDataView*>(viewPtr);
    if (!view || byteOffset < 0 || byteOffset + 8 > view->byteLength) return;

    uint64_t bits;
    memcpy(&bits, &value, 8);

    uint8_t* p = view->data + byteOffset;
    if (littleEndian) {
        p[0] = bits & 0xFF;
        p[1] = (bits >> 8) & 0xFF;
        p[2] = (bits >> 16) & 0xFF;
        p[3] = (bits >> 24) & 0xFF;
        p[4] = (bits >> 32) & 0xFF;
        p[5] = (bits >> 40) & 0xFF;
        p[6] = (bits >> 48) & 0xFF;
        p[7] = (bits >> 56) & 0xFF;
    } else {
        p[0] = (bits >> 56) & 0xFF;
        p[1] = (bits >> 48) & 0xFF;
        p[2] = (bits >> 40) & 0xFF;
        p[3] = (bits >> 32) & 0xFF;
        p[4] = (bits >> 24) & 0xFF;
        p[5] = (bits >> 16) & 0xFF;
        p[6] = (bits >> 8) & 0xFF;
        p[7] = bits & 0xFF;
    }
}

// ============================================================================
// TypedArray Higher-Order Methods (with callbacks)
// ============================================================================

// Callback function type for TypedArray methods
typedef int64_t (*TypedArrayCallbackFunc)(int64_t);
typedef int64_t (*TypedArrayReduceCallbackFunc)(int64_t, int64_t);

// Helper to get element at index based on type
static int64_t typedarray_get_element(NovaTypedArray* arr, int64_t index) {
    switch (arr->typeId) {
        case TYPED_INT8: return static_cast<int8_t*>(static_cast<void*>(arr->data))[index];
        case TYPED_UINT8:
        case TYPED_UINT8_CLAMPED: return arr->data[index];
        case TYPED_INT16: return reinterpret_cast<int16_t*>(arr->data)[index];
        case TYPED_UINT16: return reinterpret_cast<uint16_t*>(arr->data)[index];
        case TYPED_INT32: return reinterpret_cast<int32_t*>(arr->data)[index];
        case TYPED_UINT32: return reinterpret_cast<uint32_t*>(arr->data)[index];
        case TYPED_BIGINT64:
        case TYPED_BIGUINT64: return reinterpret_cast<int64_t*>(arr->data)[index];
        default: return arr->data[index];
    }
}

// Helper to set element at index based on type
static void typedarray_set_element(NovaTypedArray* arr, int64_t index, int64_t value) {
    switch (arr->typeId) {
        case TYPED_INT8: static_cast<int8_t*>(static_cast<void*>(arr->data))[index] = static_cast<int8_t>(value); break;
        case TYPED_UINT8: arr->data[index] = static_cast<uint8_t>(value); break;
        case TYPED_UINT8_CLAMPED: arr->data[index] = value < 0 ? 0 : (value > 255 ? 255 : static_cast<uint8_t>(value)); break;
        case TYPED_INT16: reinterpret_cast<int16_t*>(arr->data)[index] = static_cast<int16_t>(value); break;
        case TYPED_UINT16: reinterpret_cast<uint16_t*>(arr->data)[index] = static_cast<uint16_t>(value); break;
        case TYPED_INT32: reinterpret_cast<int32_t*>(arr->data)[index] = static_cast<int32_t>(value); break;
        case TYPED_UINT32: reinterpret_cast<uint32_t*>(arr->data)[index] = static_cast<uint32_t>(value); break;
        case TYPED_BIGINT64:
        case TYPED_BIGUINT64: reinterpret_cast<int64_t*>(arr->data)[index] = value; break;
        default: arr->data[index] = static_cast<uint8_t>(value); break;
    }
}

// TypedArray.prototype.map(callback) - returns new TypedArray with transformed elements
void* nova_typedarray_map(void* arrayPtr, TypedArrayCallbackFunc callback) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || !callback) return nova_uint8array_create(0);

    // Create new TypedArray of same type and size
    void* result = create_typed_array_new(arr->length, arr->bytesPerElement, arr->typeId);
    NovaTypedArray* resultArr = static_cast<NovaTypedArray*>(result);

    for (int64_t i = 0; i < arr->length; i++) {
        int64_t element = typedarray_get_element(arr, i);
        int64_t transformed = callback(element);
        typedarray_set_element(resultArr, i, transformed);
    }

    return result;
}

// TypedArray.prototype.filter(callback) - returns new TypedArray with matching elements
void* nova_typedarray_filter(void* arrayPtr, TypedArrayCallbackFunc callback) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || !callback) return nova_uint8array_create(0);

    // First pass: count matching elements
    int64_t matchCount = 0;
    for (int64_t i = 0; i < arr->length; i++) {
        int64_t element = typedarray_get_element(arr, i);
        if (callback(element) != 0) {
            matchCount++;
        }
    }

    // Create new TypedArray with exact size needed
    void* result = create_typed_array_new(matchCount, arr->bytesPerElement, arr->typeId);
    NovaTypedArray* resultArr = static_cast<NovaTypedArray*>(result);

    // Second pass: copy matching elements
    int64_t writeIndex = 0;
    for (int64_t i = 0; i < arr->length; i++) {
        int64_t element = typedarray_get_element(arr, i);
        if (callback(element) != 0) {
            typedarray_set_element(resultArr, writeIndex, element);
            writeIndex++;
        }
    }

    return result;
}

// TypedArray.prototype.forEach(callback) - calls callback on each element
void nova_typedarray_forEach(void* arrayPtr, TypedArrayCallbackFunc callback) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || !callback) return;

    for (int64_t i = 0; i < arr->length; i++) {
        int64_t element = typedarray_get_element(arr, i);
        callback(element);
    }
}

// TypedArray.prototype.some(callback) - returns true if any element matches
int64_t nova_typedarray_some(void* arrayPtr, TypedArrayCallbackFunc callback) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || !callback) return 0;

    for (int64_t i = 0; i < arr->length; i++) {
        int64_t element = typedarray_get_element(arr, i);
        if (callback(element) != 0) {
            return 1;  // true - at least one element matched
        }
    }
    return 0;  // false - no elements matched
}

// TypedArray.prototype.every(callback) - returns true if all elements match
int64_t nova_typedarray_every(void* arrayPtr, TypedArrayCallbackFunc callback) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || !callback) return 1;  // Empty array returns true
    if (arr->length == 0) return 1;

    for (int64_t i = 0; i < arr->length; i++) {
        int64_t element = typedarray_get_element(arr, i);
        if (callback(element) == 0) {
            return 0;  // false - at least one element didn't match
        }
    }
    return 1;  // true - all elements matched
}

// TypedArray.prototype.find(callback) - returns first matching element or undefined (0)
int64_t nova_typedarray_find(void* arrayPtr, TypedArrayCallbackFunc callback) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || !callback) return 0;

    for (int64_t i = 0; i < arr->length; i++) {
        int64_t element = typedarray_get_element(arr, i);
        if (callback(element) != 0) {
            return element;  // Return the element itself
        }
    }
    return 0;  // undefined -> 0
}

// TypedArray.prototype.findIndex(callback) - returns index of first matching element or -1
int64_t nova_typedarray_findIndex(void* arrayPtr, TypedArrayCallbackFunc callback) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || !callback) return -1;

    for (int64_t i = 0; i < arr->length; i++) {
        int64_t element = typedarray_get_element(arr, i);
        if (callback(element) != 0) {
            return i;  // Return the index
        }
    }
    return -1;  // Not found
}

// TypedArray.prototype.findLast(callback) - returns last matching element or undefined (0)
int64_t nova_typedarray_findLast(void* arrayPtr, TypedArrayCallbackFunc callback) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || !callback) return 0;

    for (int64_t i = arr->length - 1; i >= 0; i--) {
        int64_t element = typedarray_get_element(arr, i);
        if (callback(element) != 0) {
            return element;
        }
    }
    return 0;  // undefined -> 0
}

// TypedArray.prototype.findLastIndex(callback) - returns index of last matching element or -1
int64_t nova_typedarray_findLastIndex(void* arrayPtr, TypedArrayCallbackFunc callback) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || !callback) return -1;

    for (int64_t i = arr->length - 1; i >= 0; i--) {
        int64_t element = typedarray_get_element(arr, i);
        if (callback(element) != 0) {
            return i;
        }
    }
    return -1;  // Not found
}

// TypedArray.prototype.reduce(callback, initialValue) - reduces to single value
int64_t nova_typedarray_reduce(void* arrayPtr, TypedArrayReduceCallbackFunc callback, int64_t initialValue) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || !callback) return initialValue;

    int64_t accumulator = initialValue;
    for (int64_t i = 0; i < arr->length; i++) {
        int64_t element = typedarray_get_element(arr, i);
        accumulator = callback(accumulator, element);
    }
    return accumulator;
}

// TypedArray.prototype.reduceRight(callback, initialValue) - reduces right-to-left
int64_t nova_typedarray_reduceRight(void* arrayPtr, TypedArrayReduceCallbackFunc callback, int64_t initialValue) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr || !callback) return initialValue;

    int64_t accumulator = initialValue;
    for (int64_t i = arr->length - 1; i >= 0; i--) {
        int64_t element = typedarray_get_element(arr, i);
        accumulator = callback(accumulator, element);
    }
    return accumulator;
}

// ============================================================================
// TypedArray Static Methods
// ============================================================================

// TypedArray.from(arrayLike) - create TypedArray from array
// Takes a Nova array pointer and returns a new TypedArray
void* nova_int8array_from(void* arrayPtr) {
    if (!arrayPtr) return nova_int8array_create(0);
    // Get array metadata - assume it's a ValueArray with elements
    struct ValueArrayMeta { char pad[24]; int64_t length; int64_t capacity; int64_t* elements; };
    ValueArrayMeta* meta = static_cast<ValueArrayMeta*>(arrayPtr);
    int64_t len = meta->length;
    void* result = nova_int8array_create(len);
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(result);
    for (int64_t i = 0; i < len; i++) {
        arr->data[i] = static_cast<int8_t>(meta->elements[i]);
    }
    return result;
}

void* nova_uint8array_from(void* arrayPtr) {
    if (!arrayPtr) return nova_uint8array_create(0);
    struct ValueArrayMeta { char pad[24]; int64_t length; int64_t capacity; int64_t* elements; };
    ValueArrayMeta* meta = static_cast<ValueArrayMeta*>(arrayPtr);
    int64_t len = meta->length;
    void* result = nova_uint8array_create(len);
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(result);
    for (int64_t i = 0; i < len; i++) {
        arr->data[i] = static_cast<uint8_t>(meta->elements[i]);
    }
    return result;
}

void* nova_int16array_from(void* arrayPtr) {
    if (!arrayPtr) return nova_int16array_create(0);
    struct ValueArrayMeta { char pad[24]; int64_t length; int64_t capacity; int64_t* elements; };
    ValueArrayMeta* meta = static_cast<ValueArrayMeta*>(arrayPtr);
    int64_t len = meta->length;
    void* result = nova_int16array_create(len);
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(result);
    int16_t* data = reinterpret_cast<int16_t*>(arr->data);
    for (int64_t i = 0; i < len; i++) {
        data[i] = static_cast<int16_t>(meta->elements[i]);
    }
    return result;
}

void* nova_uint16array_from(void* arrayPtr) {
    if (!arrayPtr) return nova_uint16array_create(0);
    struct ValueArrayMeta { char pad[24]; int64_t length; int64_t capacity; int64_t* elements; };
    ValueArrayMeta* meta = static_cast<ValueArrayMeta*>(arrayPtr);
    int64_t len = meta->length;
    void* result = nova_uint16array_create(len);
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(result);
    uint16_t* data = reinterpret_cast<uint16_t*>(arr->data);
    for (int64_t i = 0; i < len; i++) {
        data[i] = static_cast<uint16_t>(meta->elements[i]);
    }
    return result;
}

void* nova_int32array_from(void* arrayPtr) {
    if (!arrayPtr) return nova_int32array_create(0);
    struct ValueArrayMeta { char pad[24]; int64_t length; int64_t capacity; int64_t* elements; };
    ValueArrayMeta* meta = static_cast<ValueArrayMeta*>(arrayPtr);
    int64_t len = meta->length;
    void* result = nova_int32array_create(len);
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(result);
    int32_t* data = reinterpret_cast<int32_t*>(arr->data);
    for (int64_t i = 0; i < len; i++) {
        data[i] = static_cast<int32_t>(meta->elements[i]);
    }
    return result;
}

void* nova_uint32array_from(void* arrayPtr) {
    if (!arrayPtr) return nova_uint32array_create(0);
    struct ValueArrayMeta { char pad[24]; int64_t length; int64_t capacity; int64_t* elements; };
    ValueArrayMeta* meta = static_cast<ValueArrayMeta*>(arrayPtr);
    int64_t len = meta->length;
    void* result = nova_uint32array_create(len);
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(result);
    uint32_t* data = reinterpret_cast<uint32_t*>(arr->data);
    for (int64_t i = 0; i < len; i++) {
        data[i] = static_cast<uint32_t>(meta->elements[i]);
    }
    return result;
}

void* nova_float32array_from(void* arrayPtr) {
    if (!arrayPtr) return nova_float32array_create(0);
    struct ValueArrayMeta { char pad[24]; int64_t length; int64_t capacity; int64_t* elements; };
    ValueArrayMeta* meta = static_cast<ValueArrayMeta*>(arrayPtr);
    int64_t len = meta->length;
    void* result = nova_float32array_create(len);
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(result);
    float* data = reinterpret_cast<float*>(arr->data);
    for (int64_t i = 0; i < len; i++) {
        data[i] = static_cast<float>(meta->elements[i]);
    }
    return result;
}

void* nova_float64array_from(void* arrayPtr) {
    if (!arrayPtr) return nova_float64array_create(0);
    struct ValueArrayMeta { char pad[24]; int64_t length; int64_t capacity; int64_t* elements; };
    ValueArrayMeta* meta = static_cast<ValueArrayMeta*>(arrayPtr);
    int64_t len = meta->length;
    void* result = nova_float64array_create(len);
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(result);
    double* data = reinterpret_cast<double*>(arr->data);
    for (int64_t i = 0; i < len; i++) {
        data[i] = static_cast<double>(meta->elements[i]);
    }
    return result;
}

// TypedArray.of(...elements) - create TypedArray from arguments
// For simplicity, we support up to 8 arguments
void* nova_int32array_of(int64_t count, int64_t a0, int64_t a1, int64_t a2, int64_t a3,
                         int64_t a4, int64_t a5, int64_t a6, int64_t a7) {
    void* result = nova_int32array_create(count);
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(result);
    int32_t* data = reinterpret_cast<int32_t*>(arr->data);
    int64_t args[] = {a0, a1, a2, a3, a4, a5, a6, a7};
    for (int64_t i = 0; i < count && i < 8; i++) {
        data[i] = static_cast<int32_t>(args[i]);
    }
    return result;
}

void* nova_uint8array_of(int64_t count, int64_t a0, int64_t a1, int64_t a2, int64_t a3,
                         int64_t a4, int64_t a5, int64_t a6, int64_t a7) {
    void* result = nova_uint8array_create(count);
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(result);
    int64_t args[] = {a0, a1, a2, a3, a4, a5, a6, a7};
    for (int64_t i = 0; i < count && i < 8; i++) {
        arr->data[i] = static_cast<uint8_t>(args[i]);
    }
    return result;
}

void* nova_int8array_of(int64_t count, int64_t a0, int64_t a1, int64_t a2, int64_t a3,
                        int64_t a4, int64_t a5, int64_t a6, int64_t a7) {
    void* result = nova_int8array_create(count);
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(result);
    int8_t* data = reinterpret_cast<int8_t*>(arr->data);
    int64_t args[] = {a0, a1, a2, a3, a4, a5, a6, a7};
    for (int64_t i = 0; i < count && i < 8; i++) {
        data[i] = static_cast<int8_t>(args[i]);
    }
    return result;
}

void* nova_uint8clampedarray_of(int64_t count, int64_t a0, int64_t a1, int64_t a2, int64_t a3,
                                int64_t a4, int64_t a5, int64_t a6, int64_t a7) {
    void* result = nova_uint8clampedarray_create(count);
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(result);
    int64_t args[] = {a0, a1, a2, a3, a4, a5, a6, a7};
    for (int64_t i = 0; i < count && i < 8; i++) {
        // Clamp to 0-255 range
        int64_t val = args[i];
        if (val < 0) val = 0;
        if (val > 255) val = 255;
        arr->data[i] = static_cast<uint8_t>(val);
    }
    return result;
}

void* nova_int16array_of(int64_t count, int64_t a0, int64_t a1, int64_t a2, int64_t a3,
                         int64_t a4, int64_t a5, int64_t a6, int64_t a7) {
    void* result = nova_int16array_create(count);
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(result);
    int16_t* data = reinterpret_cast<int16_t*>(arr->data);
    int64_t args[] = {a0, a1, a2, a3, a4, a5, a6, a7};
    for (int64_t i = 0; i < count && i < 8; i++) {
        data[i] = static_cast<int16_t>(args[i]);
    }
    return result;
}

void* nova_uint16array_of(int64_t count, int64_t a0, int64_t a1, int64_t a2, int64_t a3,
                          int64_t a4, int64_t a5, int64_t a6, int64_t a7) {
    void* result = nova_uint16array_create(count);
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(result);
    uint16_t* data = reinterpret_cast<uint16_t*>(arr->data);
    int64_t args[] = {a0, a1, a2, a3, a4, a5, a6, a7};
    for (int64_t i = 0; i < count && i < 8; i++) {
        data[i] = static_cast<uint16_t>(args[i]);
    }
    return result;
}

void* nova_uint32array_of(int64_t count, int64_t a0, int64_t a1, int64_t a2, int64_t a3,
                          int64_t a4, int64_t a5, int64_t a6, int64_t a7) {
    void* result = nova_uint32array_create(count);
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(result);
    uint32_t* data = reinterpret_cast<uint32_t*>(arr->data);
    int64_t args[] = {a0, a1, a2, a3, a4, a5, a6, a7};
    for (int64_t i = 0; i < count && i < 8; i++) {
        data[i] = static_cast<uint32_t>(args[i]);
    }
    return result;
}

void* nova_float32array_of(int64_t count, int64_t a0, int64_t a1, int64_t a2, int64_t a3,
                           int64_t a4, int64_t a5, int64_t a6, int64_t a7) {
    void* result = nova_float32array_create(count);
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(result);
    float* data = reinterpret_cast<float*>(arr->data);
    int64_t args[] = {a0, a1, a2, a3, a4, a5, a6, a7};
    for (int64_t i = 0; i < count && i < 8; i++) {
        data[i] = static_cast<float>(args[i]);
    }
    return result;
}

void* nova_float64array_of(int64_t count, int64_t a0, int64_t a1, int64_t a2, int64_t a3,
                           int64_t a4, int64_t a5, int64_t a6, int64_t a7) {
    void* result = nova_float64array_create(count);
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(result);
    double* data = reinterpret_cast<double*>(arr->data);
    int64_t args[] = {a0, a1, a2, a3, a4, a5, a6, a7};
    for (int64_t i = 0; i < count && i < 8; i++) {
        data[i] = static_cast<double>(args[i]);
    }
    return result;
}

// BigInt64Array.from(array) - create BigInt64Array from array
void* nova_bigint64array_from(void* arrayPtr) {
    if (!arrayPtr) return nova_bigint64array_create(0);
    struct ValueArrayMeta { char pad[24]; int64_t length; int64_t capacity; int64_t* elements; };
    ValueArrayMeta* meta = static_cast<ValueArrayMeta*>(arrayPtr);
    int64_t len = meta->length;
    void* result = nova_bigint64array_create(len);
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(result);
    int64_t* data = reinterpret_cast<int64_t*>(arr->data);
    for (int64_t i = 0; i < len; i++) {
        data[i] = meta->elements[i];
    }
    return result;
}

// BigUint64Array.from(array) - create BigUint64Array from array
void* nova_biguint64array_from(void* arrayPtr) {
    if (!arrayPtr) return nova_biguint64array_create(0);
    struct ValueArrayMeta { char pad[24]; int64_t length; int64_t capacity; int64_t* elements; };
    ValueArrayMeta* meta = static_cast<ValueArrayMeta*>(arrayPtr);
    int64_t len = meta->length;
    void* result = nova_biguint64array_create(len);
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(result);
    uint64_t* data = reinterpret_cast<uint64_t*>(arr->data);
    for (int64_t i = 0; i < len; i++) {
        data[i] = static_cast<uint64_t>(meta->elements[i]);
    }
    return result;
}

// BigInt64Array.of(...elements) - create BigInt64Array from arguments
void* nova_bigint64array_of(int64_t count, int64_t a0, int64_t a1, int64_t a2, int64_t a3,
                            int64_t a4, int64_t a5, int64_t a6, int64_t a7) {
    void* result = nova_bigint64array_create(count);
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(result);
    int64_t* data = reinterpret_cast<int64_t*>(arr->data);
    int64_t args[] = {a0, a1, a2, a3, a4, a5, a6, a7};
    for (int64_t i = 0; i < count && i < 8; i++) {
        data[i] = args[i];
    }
    return result;
}

// BigUint64Array.of(...elements) - create BigUint64Array from arguments
void* nova_biguint64array_of(int64_t count, int64_t a0, int64_t a1, int64_t a2, int64_t a3,
                             int64_t a4, int64_t a5, int64_t a6, int64_t a7) {
    void* result = nova_biguint64array_create(count);
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(result);
    uint64_t* data = reinterpret_cast<uint64_t*>(arr->data);
    int64_t args[] = {a0, a1, a2, a3, a4, a5, a6, a7};
    for (int64_t i = 0; i < count && i < 8; i++) {
        data[i] = static_cast<uint64_t>(args[i]);
    }
    return result;
}

// ============================================================================
// DataView BigInt Methods
// ============================================================================

int64_t nova_dataview_getBigInt64(void* viewPtr, int64_t byteOffset, int64_t littleEndian) {
    NovaDataView* view = static_cast<NovaDataView*>(viewPtr);
    if (!view || byteOffset < 0 || byteOffset + 8 > view->byteLength) return 0;

    uint8_t* p = view->data + byteOffset;
    int64_t result;
    if (littleEndian) {
        result = (int64_t)p[0] | ((int64_t)p[1] << 8) | ((int64_t)p[2] << 16) | ((int64_t)p[3] << 24) |
                 ((int64_t)p[4] << 32) | ((int64_t)p[5] << 40) | ((int64_t)p[6] << 48) | ((int64_t)p[7] << 56);
    } else {
        result = ((int64_t)p[0] << 56) | ((int64_t)p[1] << 48) | ((int64_t)p[2] << 40) | ((int64_t)p[3] << 32) |
                 ((int64_t)p[4] << 24) | ((int64_t)p[5] << 16) | ((int64_t)p[6] << 8) | (int64_t)p[7];
    }
    return result;
}

uint64_t nova_dataview_getBigUint64(void* viewPtr, int64_t byteOffset, int64_t littleEndian) {
    return static_cast<uint64_t>(nova_dataview_getBigInt64(viewPtr, byteOffset, littleEndian));
}

void nova_dataview_setBigInt64(void* viewPtr, int64_t byteOffset, int64_t value, int64_t littleEndian) {
    NovaDataView* view = static_cast<NovaDataView*>(viewPtr);
    if (!view || byteOffset < 0 || byteOffset + 8 > view->byteLength) return;

    uint8_t* p = view->data + byteOffset;
    if (littleEndian) {
        p[0] = value & 0xFF;
        p[1] = (value >> 8) & 0xFF;
        p[2] = (value >> 16) & 0xFF;
        p[3] = (value >> 24) & 0xFF;
        p[4] = (value >> 32) & 0xFF;
        p[5] = (value >> 40) & 0xFF;
        p[6] = (value >> 48) & 0xFF;
        p[7] = (value >> 56) & 0xFF;
    } else {
        p[0] = (value >> 56) & 0xFF;
        p[1] = (value >> 48) & 0xFF;
        p[2] = (value >> 40) & 0xFF;
        p[3] = (value >> 32) & 0xFF;
        p[4] = (value >> 24) & 0xFF;
        p[5] = (value >> 16) & 0xFF;
        p[6] = (value >> 8) & 0xFF;
        p[7] = value & 0xFF;
    }
}

void nova_dataview_setBigUint64(void* viewPtr, int64_t byteOffset, uint64_t value, int64_t littleEndian) {
    nova_dataview_setBigInt64(viewPtr, byteOffset, static_cast<int64_t>(value), littleEndian);
}

// ============================================================================
// TypedArray Iterator Methods (return arrays for compatibility with for-of)
// ============================================================================

// ValueArrayMeta structure matching nova's array metadata
struct ValueArrayMeta {
    char pad[24];
    int64_t length;
    int64_t capacity;
    int64_t* elements;
};

// Helper to create a value array with given capacity
static ValueArrayMeta* create_value_array(int64_t capacity) {
    ValueArrayMeta* meta = static_cast<ValueArrayMeta*>(malloc(sizeof(ValueArrayMeta)));
    memset(meta->pad, 0, 24);
    meta->length = 0;
    meta->capacity = capacity > 0 ? capacity : 8;
    meta->elements = static_cast<int64_t*>(malloc(meta->capacity * sizeof(int64_t)));
    return meta;
}

// Helper to push value to array
static void value_array_push(ValueArrayMeta* meta, int64_t value) {
    if (meta->length >= meta->capacity) {
        int64_t newCap = meta->capacity * 2;
        int64_t* newElems = static_cast<int64_t*>(malloc(newCap * sizeof(int64_t)));
        memcpy(newElems, meta->elements, meta->length * sizeof(int64_t));
        free(meta->elements);
        meta->elements = newElems;
        meta->capacity = newCap;
    }
    meta->elements[meta->length++] = value;
}

// TypedArray.prototype.keys() - returns array of indices
void* nova_typedarray_keys(void* arrayPtr) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr) return nullptr;

    int64_t len = arr->length;
    ValueArrayMeta* result = create_value_array(len);
    for (int64_t i = 0; i < len; i++) {
        value_array_push(result, i);
    }
    return result;
}

// TypedArray.prototype.values() - returns array of values
void* nova_typedarray_values(void* arrayPtr) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr) return nullptr;

    int64_t len = arr->length;
    ValueArrayMeta* result = create_value_array(len);
    for (int64_t i = 0; i < len; i++) {
        int64_t value = typedarray_get_element(arr, i);
        value_array_push(result, value);
    }
    return result;
}

// TypedArray.prototype.entries() - returns array of [index, value] pairs
// Each entry is a 2-element array
void* nova_typedarray_entries(void* arrayPtr) {
    NovaTypedArray* arr = static_cast<NovaTypedArray*>(arrayPtr);
    if (!arr) return nullptr;

    int64_t len = arr->length;
    ValueArrayMeta* result = create_value_array(len);
    for (int64_t i = 0; i < len; i++) {
        // Create a pair array [index, value]
        ValueArrayMeta* pair = create_value_array(2);
        value_array_push(pair, i);
        int64_t value = typedarray_get_element(arr, i);
        value_array_push(pair, value);
        // Push the pair as a pointer (cast to int64_t)
        value_array_push(result, reinterpret_cast<int64_t>(pair));
    }
    return result;
}

} // extern "C"
