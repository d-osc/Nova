/**
 * nova:buffer - Buffer Module Implementation
 *
 * Provides Buffer class for Nova programs.
 * Compatible with Node.js Buffer API.
 */

#include "nova/runtime/BuiltinModules.h"
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace nova {
namespace runtime {
namespace buffer {

// Helper to allocate and copy string
static char* allocString(const std::string& str) {
    char* result = (char*)malloc(str.length() + 1);
    if (result) {
        strcpy(result, str.c_str());
    }
    return result;
}

// Buffer structure
struct NovaBuffer {
    uint8_t* data;
    size_t length;
    size_t capacity;
};

extern "C" {

// Forward declarations
int nova_buffer_indexOf(void* buf, int value, int byteOffset);

// ============================================================================
// Static Methods - Buffer Creation
// ============================================================================

// Buffer.alloc(size[, fill[, encoding]])
void* nova_buffer_alloc(int size, int fill) {
    if (size < 0) size = 0;

    NovaBuffer* buf = (NovaBuffer*)malloc(sizeof(NovaBuffer));
    if (!buf) return nullptr;

    buf->data = (uint8_t*)malloc(size > 0 ? size : 1);
    buf->length = size;
    buf->capacity = size;

    if (buf->data && size > 0) {
        memset(buf->data, fill, size);
    }

    return buf;
}

// Buffer.allocUnsafe(size)
void* nova_buffer_allocUnsafe(int size) {
    if (size < 0) size = 0;

    NovaBuffer* buf = (NovaBuffer*)malloc(sizeof(NovaBuffer));
    if (!buf) return nullptr;

    buf->data = (uint8_t*)malloc(size > 0 ? size : 1);
    buf->length = size;
    buf->capacity = size;
    // Data is uninitialized (unsafe)

    return buf;
}

// Buffer.allocUnsafeSlow(size)
void* nova_buffer_allocUnsafeSlow(int size) {
    return nova_buffer_allocUnsafe(size);
}

// Helper: decode hex string to bytes
static size_t decodeHex(const char* hex, uint8_t* out, size_t maxLen) {
    size_t len = strlen(hex);
    size_t outLen = 0;
    for (size_t i = 0; i + 1 < len && outLen < maxLen; i += 2) {
        char byte[3] = {hex[i], hex[i + 1], 0};
        out[outLen++] = (uint8_t)strtol(byte, nullptr, 16);
    }
    return outLen;
}

// Helper: decode base64 to bytes
static size_t decodeBase64(const char* b64, uint8_t* out, size_t maxLen) {
    static const int8_t b64Table[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };
    size_t len = strlen(b64), outLen = 0;
    uint32_t buf = 0;
    int bits = 0;
    for (size_t i = 0; i < len && outLen < maxLen; i++) {
        int8_t v = b64Table[(uint8_t)b64[i]];
        if (v < 0) continue;
        buf = (buf << 6) | v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out[outLen++] = (buf >> bits) & 0xFF;
        }
    }
    return outLen;
}

// Buffer.from(string[, encoding])
void* nova_buffer_fromString(const char* str, const char* encoding) {
    if (!str) {
        return nova_buffer_alloc(0, 0);
    }

    size_t strLen = strlen(str);
    const char* enc = encoding ? encoding : "utf8";

    // Handle different encodings
    if (strcmp(enc, "hex") == 0) {
        size_t maxLen = strLen / 2;
        NovaBuffer* buf = (NovaBuffer*)nova_buffer_alloc((int)maxLen, 0);
        if (buf && buf->data) {
            buf->length = decodeHex(str, buf->data, maxLen);
        }
        return buf;
    } else if (strcmp(enc, "base64") == 0 || strcmp(enc, "base64url") == 0) {
        size_t maxLen = (strLen * 3) / 4 + 4;
        NovaBuffer* buf = (NovaBuffer*)nova_buffer_alloc((int)maxLen, 0);
        if (buf && buf->data) {
            buf->length = decodeBase64(str, buf->data, maxLen);
        }
        return buf;
    } else if (strcmp(enc, "latin1") == 0 || strcmp(enc, "binary") == 0) {
        NovaBuffer* buf = (NovaBuffer*)nova_buffer_alloc((int)strLen, 0);
        if (buf && buf->data) {
            for (size_t i = 0; i < strLen; i++) {
                buf->data[i] = (uint8_t)str[i];
            }
        }
        return buf;
    } else if (strcmp(enc, "ascii") == 0) {
        NovaBuffer* buf = (NovaBuffer*)nova_buffer_alloc((int)strLen, 0);
        if (buf && buf->data) {
            for (size_t i = 0; i < strLen; i++) {
                buf->data[i] = (uint8_t)(str[i] & 0x7F);
            }
        }
        return buf;
    }

    // Default: utf8 (or utf-8)
    NovaBuffer* buf = (NovaBuffer*)nova_buffer_alloc((int)strLen, 0);
    if (buf && buf->data) {
        memcpy(buf->data, str, strLen);
    }
    return buf;
}

// Buffer.from(array)
void* nova_buffer_fromArray(const uint8_t* arr, int length) {
    if (!arr || length <= 0) {
        return nova_buffer_alloc(0, 0);
    }

    NovaBuffer* buf = (NovaBuffer*)nova_buffer_alloc(length, 0);
    if (buf && buf->data) {
        memcpy(buf->data, arr, length);
    }

    return buf;
}

// Buffer.from(buffer)
void* nova_buffer_fromBuffer(void* srcBuf) {
    if (!srcBuf) return nova_buffer_alloc(0, 0);

    NovaBuffer* src = (NovaBuffer*)srcBuf;
    NovaBuffer* buf = (NovaBuffer*)nova_buffer_alloc((int)src->length, 0);
    if (buf && buf->data && src->data) {
        memcpy(buf->data, src->data, src->length);
    }

    return buf;
}

// Buffer.byteLength(string[, encoding])
int nova_buffer_byteLength(const char* str, const char* encoding) {
    (void)encoding;
    return str ? (int)strlen(str) : 0;
}

// Buffer.compare(buf1, buf2)
int nova_buffer_compare(void* buf1, void* buf2) {
    if (!buf1 && !buf2) return 0;
    if (!buf1) return -1;
    if (!buf2) return 1;

    NovaBuffer* b1 = (NovaBuffer*)buf1;
    NovaBuffer* b2 = (NovaBuffer*)buf2;

    size_t minLen = std::min(b1->length, b2->length);
    int cmp = memcmp(b1->data, b2->data, minLen);

    if (cmp != 0) return cmp;
    if (b1->length < b2->length) return -1;
    if (b1->length > b2->length) return 1;
    return 0;
}

// Buffer.concat(list[, totalLength])
void* nova_buffer_concat(void** buffers, int count, int totalLength) {
    if (!buffers || count <= 0) {
        return nova_buffer_alloc(0, 0);
    }

    // Calculate total length if not provided
    if (totalLength < 0) {
        totalLength = 0;
        for (int i = 0; i < count; i++) {
            if (buffers[i]) {
                totalLength += (int)((NovaBuffer*)buffers[i])->length;
            }
        }
    }

    NovaBuffer* result = (NovaBuffer*)nova_buffer_alloc(totalLength, 0);
    if (!result || !result->data) return result;

    size_t offset = 0;
    for (int i = 0; i < count && offset < (size_t)totalLength; i++) {
        if (buffers[i]) {
            NovaBuffer* buf = (NovaBuffer*)buffers[i];
            size_t copyLen = std::min(buf->length, (size_t)totalLength - offset);
            memcpy(result->data + offset, buf->data, copyLen);
            offset += copyLen;
        }
    }

    return result;
}

// Buffer.isBuffer(obj)
int nova_buffer_isBuffer(void* obj) {
    return obj != nullptr ? 1 : 0;
}

// Buffer.isEncoding(encoding)
int nova_buffer_isEncoding(const char* encoding) {
    if (!encoding) return 0;

    const char* valid[] = {"utf8", "utf-8", "ascii", "binary", "base64",
                           "hex", "latin1", "ucs2", "ucs-2", "utf16le", "utf-16le"};
    for (int i = 0; i < 11; i++) {
        if (_stricmp(encoding, valid[i]) == 0) return 1;
    }
    return 0;
}

// ============================================================================
// Instance Properties
// ============================================================================

// buffer.length
int nova_buffer_length(void* buf) {
    return buf ? (int)((NovaBuffer*)buf)->length : 0;
}

// Get raw data pointer
uint8_t* nova_buffer_data(void* buf) {
    return buf ? ((NovaBuffer*)buf)->data : nullptr;
}

// ============================================================================
// Instance Methods - Read
// ============================================================================

int nova_buffer_readInt8(void* buf, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset >= b->length) return 0;
    return (int8_t)b->data[offset];
}

int nova_buffer_readUInt8(void* buf, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset >= b->length) return 0;
    return b->data[offset];
}

int nova_buffer_readInt16LE(void* buf, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 2 > b->length) return 0;
    return (int16_t)(b->data[offset] | (b->data[offset + 1] << 8));
}

int nova_buffer_readInt16BE(void* buf, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 2 > b->length) return 0;
    return (int16_t)((b->data[offset] << 8) | b->data[offset + 1]);
}

int nova_buffer_readUInt16LE(void* buf, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 2 > b->length) return 0;
    return (uint16_t)(b->data[offset] | (b->data[offset + 1] << 8));
}

int nova_buffer_readUInt16BE(void* buf, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 2 > b->length) return 0;
    return (uint16_t)((b->data[offset] << 8) | b->data[offset + 1]);
}

int nova_buffer_readInt32LE(void* buf, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 4 > b->length) return 0;
    return (int32_t)(b->data[offset] | (b->data[offset + 1] << 8) |
                     (b->data[offset + 2] << 16) | (b->data[offset + 3] << 24));
}

int nova_buffer_readInt32BE(void* buf, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 4 > b->length) return 0;
    return (int32_t)((b->data[offset] << 24) | (b->data[offset + 1] << 16) |
                     (b->data[offset + 2] << 8) | b->data[offset + 3]);
}

uint32_t nova_buffer_readUInt32LE(void* buf, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 4 > b->length) return 0;
    return (uint32_t)(b->data[offset] | (b->data[offset + 1] << 8) |
                      (b->data[offset + 2] << 16) | (b->data[offset + 3] << 24));
}

uint32_t nova_buffer_readUInt32BE(void* buf, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 4 > b->length) return 0;
    return (uint32_t)((b->data[offset] << 24) | (b->data[offset + 1] << 16) |
                      (b->data[offset + 2] << 8) | b->data[offset + 3]);
}

int64_t nova_buffer_readBigInt64LE(void* buf, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 8 > b->length) return 0;
    int64_t result = 0;
    for (int i = 0; i < 8; i++) {
        result |= ((int64_t)b->data[offset + i]) << (i * 8);
    }
    return result;
}

int64_t nova_buffer_readBigInt64BE(void* buf, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 8 > b->length) return 0;
    int64_t result = 0;
    for (int i = 0; i < 8; i++) {
        result |= ((int64_t)b->data[offset + i]) << ((7 - i) * 8);
    }
    return result;
}

uint64_t nova_buffer_readBigUInt64LE(void* buf, int offset) {
    return (uint64_t)nova_buffer_readBigInt64LE(buf, offset);
}

uint64_t nova_buffer_readBigUInt64BE(void* buf, int offset) {
    return (uint64_t)nova_buffer_readBigInt64BE(buf, offset);
}

float nova_buffer_readFloatLE(void* buf, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 4 > b->length) return 0;
    float result;
    memcpy(&result, b->data + offset, 4);
    return result;
}

float nova_buffer_readFloatBE(void* buf, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 4 > b->length) return 0;
    uint8_t reversed[4];
    for (int i = 0; i < 4; i++) reversed[i] = b->data[offset + 3 - i];
    float result;
    memcpy(&result, reversed, 4);
    return result;
}

double nova_buffer_readDoubleLE(void* buf, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 8 > b->length) return 0;
    double result;
    memcpy(&result, b->data + offset, 8);
    return result;
}

double nova_buffer_readDoubleBE(void* buf, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 8 > b->length) return 0;
    uint8_t reversed[8];
    for (int i = 0; i < 8; i++) reversed[i] = b->data[offset + 7 - i];
    double result;
    memcpy(&result, reversed, 8);
    return result;
}

// ============================================================================
// Instance Methods - Write
// ============================================================================

int nova_buffer_writeInt8(void* buf, int value, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset >= b->length) return offset;
    b->data[offset] = (uint8_t)value;
    return offset + 1;
}

int nova_buffer_writeUInt8(void* buf, int value, int offset) {
    return nova_buffer_writeInt8(buf, value, offset);
}

int nova_buffer_writeInt16LE(void* buf, int value, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 2 > b->length) return offset;
    b->data[offset] = value & 0xff;
    b->data[offset + 1] = (value >> 8) & 0xff;
    return offset + 2;
}

int nova_buffer_writeInt16BE(void* buf, int value, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 2 > b->length) return offset;
    b->data[offset] = (value >> 8) & 0xff;
    b->data[offset + 1] = value & 0xff;
    return offset + 2;
}

int nova_buffer_writeUInt16LE(void* buf, int value, int offset) {
    return nova_buffer_writeInt16LE(buf, value, offset);
}

int nova_buffer_writeUInt16BE(void* buf, int value, int offset) {
    return nova_buffer_writeInt16BE(buf, value, offset);
}

int nova_buffer_writeInt32LE(void* buf, int value, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 4 > b->length) return offset;
    b->data[offset] = value & 0xff;
    b->data[offset + 1] = (value >> 8) & 0xff;
    b->data[offset + 2] = (value >> 16) & 0xff;
    b->data[offset + 3] = (value >> 24) & 0xff;
    return offset + 4;
}

int nova_buffer_writeInt32BE(void* buf, int value, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 4 > b->length) return offset;
    b->data[offset] = (value >> 24) & 0xff;
    b->data[offset + 1] = (value >> 16) & 0xff;
    b->data[offset + 2] = (value >> 8) & 0xff;
    b->data[offset + 3] = value & 0xff;
    return offset + 4;
}

int nova_buffer_writeUInt32LE(void* buf, uint32_t value, int offset) {
    return nova_buffer_writeInt32LE(buf, (int)value, offset);
}

int nova_buffer_writeUInt32BE(void* buf, uint32_t value, int offset) {
    return nova_buffer_writeInt32BE(buf, (int)value, offset);
}

int nova_buffer_writeBigInt64LE(void* buf, int64_t value, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 8 > b->length) return offset;
    for (int i = 0; i < 8; i++) {
        b->data[offset + i] = (value >> (i * 8)) & 0xff;
    }
    return offset + 8;
}

int nova_buffer_writeBigInt64BE(void* buf, int64_t value, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 8 > b->length) return offset;
    for (int i = 0; i < 8; i++) {
        b->data[offset + i] = (value >> ((7 - i) * 8)) & 0xff;
    }
    return offset + 8;
}

int nova_buffer_writeBigUInt64LE(void* buf, uint64_t value, int offset) {
    return nova_buffer_writeBigInt64LE(buf, (int64_t)value, offset);
}

int nova_buffer_writeBigUInt64BE(void* buf, uint64_t value, int offset) {
    return nova_buffer_writeBigInt64BE(buf, (int64_t)value, offset);
}

int nova_buffer_writeFloatLE(void* buf, float value, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 4 > b->length) return offset;
    memcpy(b->data + offset, &value, 4);
    return offset + 4;
}

int nova_buffer_writeFloatBE(void* buf, float value, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 4 > b->length) return offset;
    uint8_t bytes[4];
    memcpy(bytes, &value, 4);
    for (int i = 0; i < 4; i++) b->data[offset + i] = bytes[3 - i];
    return offset + 4;
}

int nova_buffer_writeDoubleLE(void* buf, double value, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 8 > b->length) return offset;
    memcpy(b->data + offset, &value, 8);
    return offset + 8;
}

int nova_buffer_writeDoubleBE(void* buf, double value, int offset) {
    if (!buf) return 0;
    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0 || (size_t)offset + 8 > b->length) return offset;
    uint8_t bytes[8];
    memcpy(bytes, &value, 8);
    for (int i = 0; i < 8; i++) b->data[offset + i] = bytes[7 - i];
    return offset + 8;
}

// ============================================================================
// Instance Methods - Operations
// ============================================================================

// buffer.copy(target[, targetStart[, sourceStart[, sourceEnd]]])
int nova_buffer_copy(void* src, void* target, int targetStart, int sourceStart, int sourceEnd) {
    if (!src || !target) return 0;

    NovaBuffer* s = (NovaBuffer*)src;
    NovaBuffer* t = (NovaBuffer*)target;

    if (sourceStart < 0) sourceStart = 0;
    if (sourceEnd < 0 || (size_t)sourceEnd > s->length) sourceEnd = (int)s->length;
    if (targetStart < 0) targetStart = 0;

    int copyLen = sourceEnd - sourceStart;
    if ((size_t)(targetStart + copyLen) > t->length) {
        copyLen = (int)t->length - targetStart;
    }

    if (copyLen > 0) {
        memmove(t->data + targetStart, s->data + sourceStart, copyLen);
    }

    return copyLen;
}

// buffer.equals(otherBuffer)
int nova_buffer_equals(void* buf1, void* buf2) {
    return nova_buffer_compare(buf1, buf2) == 0 ? 1 : 0;
}

// buffer.fill(value[, offset[, end]])
void* nova_buffer_fill(void* buf, int value, int offset, int end) {
    if (!buf) return buf;

    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0) offset = 0;
    if (end < 0 || (size_t)end > b->length) end = (int)b->length;

    if (offset < end) {
        memset(b->data + offset, value, end - offset);
    }

    return buf;
}

// buffer.includes(value[, byteOffset])
int nova_buffer_includes(void* buf, int value, int byteOffset) {
    return nova_buffer_indexOf(buf, value, byteOffset) >= 0 ? 1 : 0;
}

// buffer.indexOf(value[, byteOffset])
int nova_buffer_indexOf(void* buf, int value, int byteOffset) {
    if (!buf) return -1;

    NovaBuffer* b = (NovaBuffer*)buf;
    if (byteOffset < 0) byteOffset = 0;

    for (size_t i = byteOffset; i < b->length; i++) {
        if (b->data[i] == (uint8_t)value) {
            return (int)i;
        }
    }

    return -1;
}

// buffer.lastIndexOf(value[, byteOffset])
int nova_buffer_lastIndexOf(void* buf, int value, int byteOffset) {
    if (!buf) return -1;

    NovaBuffer* b = (NovaBuffer*)buf;
    if (byteOffset < 0 || (size_t)byteOffset >= b->length) {
        byteOffset = (int)b->length - 1;
    }

    for (int i = byteOffset; i >= 0; i--) {
        if (b->data[i] == (uint8_t)value) {
            return i;
        }
    }

    return -1;
}

// buffer.slice([start[, end]]) - returns new buffer
void* nova_buffer_slice(void* buf, int start, int end) {
    if (!buf) return nova_buffer_alloc(0, 0);

    NovaBuffer* b = (NovaBuffer*)buf;
    if (start < 0) start = (int)b->length + start;
    if (end < 0) end = (int)b->length + end;
    if (start < 0) start = 0;
    if ((size_t)end > b->length) end = (int)b->length;
    if (start >= end) return nova_buffer_alloc(0, 0);

    int len = end - start;
    NovaBuffer* result = (NovaBuffer*)nova_buffer_alloc(len, 0);
    if (result && result->data) {
        memcpy(result->data, b->data + start, len);
    }

    return result;
}

// buffer.subarray([start[, end]]) - same as slice
void* nova_buffer_subarray(void* buf, int start, int end) {
    return nova_buffer_slice(buf, start, end);
}

// buffer.swap16()
void* nova_buffer_swap16(void* buf) {
    if (!buf) return buf;
    NovaBuffer* b = (NovaBuffer*)buf;
    for (size_t i = 0; i + 1 < b->length; i += 2) {
        uint8_t tmp = b->data[i];
        b->data[i] = b->data[i + 1];
        b->data[i + 1] = tmp;
    }
    return buf;
}

// buffer.swap32()
void* nova_buffer_swap32(void* buf) {
    if (!buf) return buf;
    NovaBuffer* b = (NovaBuffer*)buf;
    for (size_t i = 0; i + 3 < b->length; i += 4) {
        uint8_t tmp0 = b->data[i], tmp1 = b->data[i + 1];
        b->data[i] = b->data[i + 3];
        b->data[i + 1] = b->data[i + 2];
        b->data[i + 2] = tmp1;
        b->data[i + 3] = tmp0;
    }
    return buf;
}

// buffer.swap64()
void* nova_buffer_swap64(void* buf) {
    if (!buf) return buf;
    NovaBuffer* b = (NovaBuffer*)buf;
    for (size_t i = 0; i + 7 < b->length; i += 8) {
        for (int j = 0; j < 4; j++) {
            uint8_t tmp = b->data[i + j];
            b->data[i + j] = b->data[i + 7 - j];
            b->data[i + 7 - j] = tmp;
        }
    }
    return buf;
}

// buffer.reverse()
void* nova_buffer_reverse(void* buf) {
    if (!buf) return buf;
    NovaBuffer* b = (NovaBuffer*)buf;
    for (size_t i = 0; i < b->length / 2; i++) {
        uint8_t tmp = b->data[i];
        b->data[i] = b->data[b->length - 1 - i];
        b->data[b->length - 1 - i] = tmp;
    }
    return buf;
}

// buffer.toString([encoding[, start[, end]]])
char* nova_buffer_toString(void* buf, const char* encoding, int start, int end) {
    (void)encoding;
    if (!buf) return allocString("");

    NovaBuffer* b = (NovaBuffer*)buf;
    if (start < 0) start = 0;
    if (end < 0 || (size_t)end > b->length) end = (int)b->length;
    if (start >= end) return allocString("");

    int len = end - start;
    char* result = (char*)malloc(len + 1);
    if (result) {
        memcpy(result, b->data + start, len);
        result[len] = '\0';
    }
    return result;
}

// buffer.toJSON()
char* nova_buffer_toJSON(void* buf) {
    if (!buf) return allocString("{\"type\":\"Buffer\",\"data\":[]}");

    NovaBuffer* b = (NovaBuffer*)buf;
    std::ostringstream oss;
    oss << "{\"type\":\"Buffer\",\"data\":[";
    for (size_t i = 0; i < b->length; i++) {
        if (i > 0) oss << ",";
        oss << (int)b->data[i];
    }
    oss << "]}";
    return allocString(oss.str());
}

// buffer.write(string[, offset[, length]][, encoding])
int nova_buffer_write(void* buf, const char* str, int offset, int length, const char* encoding) {
    (void)encoding;
    if (!buf || !str) return 0;

    NovaBuffer* b = (NovaBuffer*)buf;
    if (offset < 0) offset = 0;
    if ((size_t)offset >= b->length) return 0;

    size_t strLen = strlen(str);
    if (length < 0 || (size_t)length > strLen) length = (int)strLen;
    if ((size_t)(offset + length) > b->length) length = (int)b->length - offset;

    memcpy(b->data + offset, str, length);
    return length;
}

// ============================================================================
// Memory Management
// ============================================================================

void nova_buffer_free(void* buf) {
    if (buf) {
        NovaBuffer* b = (NovaBuffer*)buf;
        if (b->data) free(b->data);
        free(b);
    }
}

// ============================================================================
// Hex/Base64 Conversion
// ============================================================================

char* nova_buffer_toHex(void* buf) {
    if (!buf) return allocString("");
    NovaBuffer* b = (NovaBuffer*)buf;

    std::ostringstream oss;
    for (size_t i = 0; i < b->length; i++) {
        oss << std::hex << std::setfill('0') << std::setw(2) << (int)b->data[i];
    }
    return allocString(oss.str());
}

void* nova_buffer_fromHex(const char* hex) {
    if (!hex) return nova_buffer_alloc(0, 0);

    size_t len = strlen(hex);
    if (len % 2 != 0) return nova_buffer_alloc(0, 0);

    NovaBuffer* buf = (NovaBuffer*)nova_buffer_alloc((int)(len / 2), 0);
    if (!buf || !buf->data) return buf;

    for (size_t i = 0; i < len / 2; i++) {
        unsigned int byte;
        sscanf(hex + i * 2, "%2x", &byte);
        buf->data[i] = (uint8_t)byte;
    }

    return buf;
}

} // extern "C"

} // namespace buffer
} // namespace runtime
} // namespace nova
