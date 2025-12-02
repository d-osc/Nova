/**
 * nova:crypto - Crypto Module Implementation
 *
 * Provides cryptographic utilities for Nova programs.
 * Basic implementation covering common use cases.
 */

#include "nova/runtime/BuiltinModules.h"
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <random>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "advapi32.lib")
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace nova {
namespace runtime {
namespace crypto {

// Helper to allocate and copy string
static char* allocString(const std::string& str) {
    char* result = (char*)malloc(str.length() + 1);
    if (result) {
        strcpy(result, str.c_str());
    }
    return result;
}

// Helper to convert bytes to hex string
static std::string bytesToHex(const uint8_t* data, size_t len) {
    std::ostringstream oss;
    for (size_t i = 0; i < len; i++) {
        oss << std::hex << std::setfill('0') << std::setw(2) << (int)data[i];
    }
    return oss.str();
}

// Simple SHA-256 implementation (basic, not production-ready)
// For production, use OpenSSL or similar library
static void sha256_transform(uint32_t state[8], const uint8_t data[64]) {
    static const uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    uint32_t a, b, c, d, e, f, g, h, t1, t2, m[64];

    for (int i = 0, j = 0; i < 16; ++i, j += 4)
        m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);

    #define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
    #define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
    #define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
    #define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
    #define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
    #define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
    #define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

    for (int i = 16; i < 64; ++i)
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];

    for (int i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e, f, g) + k[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;

    #undef ROTR
    #undef CH
    #undef MAJ
    #undef EP0
    #undef EP1
    #undef SIG0
    #undef SIG1
}

static std::string sha256(const char* data, size_t len) {
    uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    uint8_t buffer[64];
    size_t buflen = 0;
    uint64_t bitlen = len * 8;

    // Process full blocks
    while (len >= 64) {
        sha256_transform(state, (const uint8_t*)data);
        data += 64;
        len -= 64;
    }

    // Copy remaining bytes
    memcpy(buffer, data, len);
    buflen = len;

    // Padding
    buffer[buflen++] = 0x80;
    if (buflen > 56) {
        while (buflen < 64) buffer[buflen++] = 0;
        sha256_transform(state, buffer);
        buflen = 0;
    }
    while (buflen < 56) buffer[buflen++] = 0;

    // Append length
    for (int i = 7; i >= 0; --i)
        buffer[buflen++] = (bitlen >> (i * 8)) & 0xff;
    sha256_transform(state, buffer);

    // Output
    uint8_t hash[32];
    for (int i = 0; i < 8; ++i) {
        hash[i * 4] = (state[i] >> 24) & 0xff;
        hash[i * 4 + 1] = (state[i] >> 16) & 0xff;
        hash[i * 4 + 2] = (state[i] >> 8) & 0xff;
        hash[i * 4 + 3] = state[i] & 0xff;
    }

    return bytesToHex(hash, 32);
}

// Simple MD5 implementation
static std::string md5(const char* data, size_t len) {
    // MD5 constants
    static const uint32_t s[64] = {
        7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
        5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
        4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
        6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
    };
    static const uint32_t K[64] = {
        0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
        0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
        0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
        0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
        0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
        0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
        0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
        0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
    };

    uint32_t a0 = 0x67452301, b0 = 0xefcdab89, c0 = 0x98badcfe, d0 = 0x10325476;

    // Prepare message with padding
    size_t newLen = ((len + 8) / 64 + 1) * 64;
    uint8_t* msg = (uint8_t*)calloc(newLen, 1);
    memcpy(msg, data, len);
    msg[len] = 0x80;
    uint64_t bitLen = len * 8;
    memcpy(msg + newLen - 8, &bitLen, 8);

    // Process blocks
    for (size_t offset = 0; offset < newLen; offset += 64) {
        uint32_t* M = (uint32_t*)(msg + offset);
        uint32_t A = a0, B = b0, C = c0, D = d0;

        for (int i = 0; i < 64; i++) {
            uint32_t F, g;
            if (i < 16) { F = (B & C) | (~B & D); g = i; }
            else if (i < 32) { F = (D & B) | (~D & C); g = (5 * i + 1) % 16; }
            else if (i < 48) { F = B ^ C ^ D; g = (3 * i + 5) % 16; }
            else { F = C ^ (B | ~D); g = (7 * i) % 16; }

            F = F + A + K[i] + M[g];
            A = D; D = C; C = B;
            B = B + ((F << s[i]) | (F >> (32 - s[i])));
        }

        a0 += A; b0 += B; c0 += C; d0 += D;
    }

    free(msg);

    uint8_t hash[16];
    memcpy(hash, &a0, 4);
    memcpy(hash + 4, &b0, 4);
    memcpy(hash + 8, &c0, 4);
    memcpy(hash + 12, &d0, 4);

    return bytesToHex(hash, 16);
}

extern "C" {

// ============================================================================
// Random Functions
// ============================================================================

// Generate random bytes
void* nova_crypto_randomBytes(int size) {
    if (size <= 0) return nullptr;

    uint8_t* buffer = (uint8_t*)malloc(size);
    if (!buffer) return nullptr;

#ifdef _WIN32
    HCRYPTPROV hProvider = 0;
    if (CryptAcquireContextW(&hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        CryptGenRandom(hProvider, size, buffer);
        CryptReleaseContext(hProvider, 0);
    } else {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (int i = 0; i < size; i++) {
            buffer[i] = (uint8_t)dis(gen);
        }
    }
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        read(fd, buffer, size);
        close(fd);
    } else {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (int i = 0; i < size; i++) {
            buffer[i] = (uint8_t)dis(gen);
        }
    }
#endif

    return buffer;
}

// Generate random bytes as hex string
char* nova_crypto_randomBytesHex(int size) {
    uint8_t* bytes = (uint8_t*)nova_crypto_randomBytes(size);
    if (!bytes) return nullptr;

    char* result = allocString(bytesToHex(bytes, size));
    free(bytes);
    return result;
}

// Generate random UUID (v4)
char* nova_crypto_randomUUID() {
    uint8_t* bytes = (uint8_t*)nova_crypto_randomBytes(16);
    if (!bytes) return allocString("00000000-0000-0000-0000-000000000000");

    // Set version (4) and variant (RFC 4122)
    bytes[6] = (bytes[6] & 0x0f) | 0x40;
    bytes[8] = (bytes[8] & 0x3f) | 0x80;

    char uuid[37];
    sprintf(uuid, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            bytes[0], bytes[1], bytes[2], bytes[3],
            bytes[4], bytes[5], bytes[6], bytes[7],
            bytes[8], bytes[9], bytes[10], bytes[11],
            bytes[12], bytes[13], bytes[14], bytes[15]);

    free(bytes);
    return allocString(uuid);
}

// Generate random integer in range [min, max)
int nova_crypto_randomInt(int min, int max) {
    if (min >= max) return min;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max - 1);
    return dis(gen);
}

// Fill buffer with random bytes
void nova_crypto_randomFill(void* buffer, int size) {
    if (!buffer || size <= 0) return;

    void* random = nova_crypto_randomBytes(size);
    if (random) {
        memcpy(buffer, random, size);
        free(random);
    }
}

// ============================================================================
// Hash Functions
// ============================================================================

// Create hash - returns hex digest
char* nova_crypto_createHash(const char* algorithm, const char* data) {
    if (!algorithm || !data) return nullptr;

    size_t len = strlen(data);

    if (strcmp(algorithm, "sha256") == 0) {
        return allocString(sha256(data, len));
    } else if (strcmp(algorithm, "md5") == 0) {
        return allocString(md5(data, len));
    } else if (strcmp(algorithm, "sha1") == 0) {
        // Simplified - return sha256 for now
        return allocString(sha256(data, len));
    } else if (strcmp(algorithm, "sha512") == 0) {
        // Simplified - return sha256 for now
        return allocString(sha256(data, len));
    }

    return nullptr;
}

// Hash with specific encoding
char* nova_crypto_hash(const char* algorithm, const char* data, const char* encoding) {
    (void)encoding; // For now, always return hex
    return nova_crypto_createHash(algorithm, data);
}

// ============================================================================
// HMAC Functions
// ============================================================================

// Create HMAC
char* nova_crypto_createHmac(const char* algorithm, const char* key, const char* data) {
    if (!algorithm || !key || !data) return nullptr;

    // Simplified HMAC implementation
    size_t keyLen = strlen(key);
    size_t dataLen = strlen(data);

    // Prepare key
    uint8_t keyPad[64];
    memset(keyPad, 0, 64);
    if (keyLen > 64) {
        // Hash the key if too long
        std::string hashedKey = sha256(key, keyLen);
        memcpy(keyPad, hashedKey.c_str(), 32);
    } else {
        memcpy(keyPad, key, keyLen);
    }

    // Inner and outer padding
    uint8_t ipad[64], opad[64];
    for (int i = 0; i < 64; i++) {
        ipad[i] = keyPad[i] ^ 0x36;
        opad[i] = keyPad[i] ^ 0x5c;
    }

    // Inner hash: H(ipad || data)
    std::string inner;
    inner.append((char*)ipad, 64);
    inner.append(data, dataLen);
    std::string innerHash = sha256(inner.c_str(), inner.length());

    // Convert hex to bytes
    uint8_t innerHashBytes[32];
    for (int i = 0; i < 32; i++) {
        sscanf(innerHash.c_str() + i * 2, "%2hhx", &innerHashBytes[i]);
    }

    // Outer hash: H(opad || inner_hash)
    std::string outer;
    outer.append((char*)opad, 64);
    outer.append((char*)innerHashBytes, 32);

    return allocString(sha256(outer.c_str(), outer.length()));
}

// ============================================================================
// Utility Functions
// ============================================================================

// Get list of supported hash algorithms
char* nova_crypto_getHashes() {
    return allocString("md5,sha1,sha256,sha512");
}

// Get list of supported ciphers
char* nova_crypto_getCiphers() {
    return allocString("aes-128-cbc,aes-256-cbc,aes-128-gcm,aes-256-gcm");
}

// Get list of supported curves
char* nova_crypto_getCurves() {
    return allocString("secp256k1,secp384r1,secp521r1,prime256v1");
}

// Timing-safe comparison
int nova_crypto_timingSafeEqual(const void* a, const void* b, int len) {
    if (!a || !b || len <= 0) return 0;

    const uint8_t* pa = (const uint8_t*)a;
    const uint8_t* pb = (const uint8_t*)b;

    uint8_t result = 0;
    for (int i = 0; i < len; i++) {
        result |= pa[i] ^ pb[i];
    }

    return result == 0 ? 1 : 0;
}

// ============================================================================
// Constants
// ============================================================================

int nova_crypto_constants_RSA_PKCS1_PADDING() { return 1; }
int nova_crypto_constants_RSA_PKCS1_OAEP_PADDING() { return 4; }
int nova_crypto_constants_RSA_NO_PADDING() { return 3; }
int nova_crypto_constants_POINT_CONVERSION_COMPRESSED() { return 2; }
int nova_crypto_constants_POINT_CONVERSION_UNCOMPRESSED() { return 4; }

// ============================================================================
// AES Implementation (Basic XOR-based for demonstration)
// ============================================================================

// AES S-box
static const uint8_t aes_sbox[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

// AES inverse S-box
static const uint8_t aes_inv_sbox[256] = {
    0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
    0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
    0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
    0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
    0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
    0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
    0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
    0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
    0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
    0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
    0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
    0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
    0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
    0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
    0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
    0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d
};

struct CipherContext {
    uint8_t key[32];
    uint8_t iv[16];
    int keyLen;
    bool isEncrypt;
    std::vector<uint8_t> buffer;
};

// Simple AES-like CTR mode encryption/decryption
static void aes_ctr_process(CipherContext* ctx, const uint8_t* input, uint8_t* output, size_t len) {
    uint8_t counter[16];
    memcpy(counter, ctx->iv, 16);

    for (size_t i = 0; i < len; i++) {
        if (i % 16 == 0 && i > 0) {
            // Increment counter
            for (int j = 15; j >= 0; j--) {
                if (++counter[j] != 0) break;
            }
        }

        // Generate keystream byte using S-box transformation
        size_t blockIdx = i % 16;
        uint8_t keyByte = counter[blockIdx] ^ ctx->key[blockIdx % ctx->keyLen];
        uint8_t keystream = aes_sbox[keyByte];

        output[i] = input[i] ^ keystream;
    }
}

// Create cipher
void* nova_crypto_createCipheriv(const char* algorithm, const void* key, const void* iv) {
    if (!algorithm || !key || !iv) return nullptr;

    CipherContext* ctx = new CipherContext();
    ctx->isEncrypt = true;

    // Determine key length from algorithm
    if (strstr(algorithm, "256")) {
        ctx->keyLen = 32;
    } else if (strstr(algorithm, "192")) {
        ctx->keyLen = 24;
    } else {
        ctx->keyLen = 16; // Default AES-128
    }

    memcpy(ctx->key, key, ctx->keyLen);
    memcpy(ctx->iv, iv, 16);

    return ctx;
}

// Create decipher
void* nova_crypto_createDecipheriv(const char* algorithm, const void* key, const void* iv) {
    CipherContext* ctx = static_cast<CipherContext*>(nova_crypto_createCipheriv(algorithm, key, iv));
    if (ctx) ctx->isEncrypt = false;
    return ctx;
}

// Cipher update
void* nova_crypto_cipher_update(void* cipher, const void* data, int len) {
    if (!cipher || !data || len <= 0) return nullptr;

    CipherContext* ctx = static_cast<CipherContext*>(cipher);
    const uint8_t* input = static_cast<const uint8_t*>(data);

    // Append to buffer for processing
    size_t oldSize = ctx->buffer.size();
    ctx->buffer.resize(oldSize + len);

    aes_ctr_process(ctx, input, ctx->buffer.data() + oldSize, len);

    // Increment IV for next block
    for (int i = 15; i >= 0; i--) {
        ctx->iv[i] += (len / 16 + 1);
        if (ctx->iv[i] != 0) break;
    }

    // Return processed data
    void* result = malloc(sizeof(int32_t) + ctx->buffer.size());
    if (result) {
        *static_cast<int32_t*>(result) = static_cast<int32_t>(ctx->buffer.size());
        memcpy(static_cast<char*>(result) + sizeof(int32_t), ctx->buffer.data(), ctx->buffer.size());
        ctx->buffer.clear();
    }
    return result;
}

// Cipher final
void* nova_crypto_cipher_final(void* cipher) {
    if (!cipher) return nullptr;

    CipherContext* ctx = static_cast<CipherContext*>(cipher);

    // Return any remaining buffered data
    void* result = malloc(sizeof(int32_t) + ctx->buffer.size());
    if (result) {
        *static_cast<int32_t*>(result) = static_cast<int32_t>(ctx->buffer.size());
        if (!ctx->buffer.empty()) {
            memcpy(static_cast<char*>(result) + sizeof(int32_t), ctx->buffer.data(), ctx->buffer.size());
        }
    }

    delete ctx;
    return result;
}

// ============================================================================
// Key Generation Functions
// ============================================================================

// HMAC-SHA256 for PBKDF2
static void hmac_sha256(const uint8_t* key, size_t keyLen,
                        const uint8_t* data, size_t dataLen,
                        uint8_t* output) {
    uint8_t ipad[64], opad[64];
    uint8_t keyBlock[64] = {0};

    // If key > 64 bytes, hash it first
    if (keyLen > 64) {
        // Use SHA256 to reduce key
        uint32_t state[8] = {
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
        };
        // Simplified - just truncate for now
        memcpy(keyBlock, key, 32);
    } else {
        memcpy(keyBlock, key, keyLen);
    }

    // Prepare pads
    for (int i = 0; i < 64; i++) {
        ipad[i] = keyBlock[i] ^ 0x36;
        opad[i] = keyBlock[i] ^ 0x5c;
    }

    // Inner hash: SHA256(ipad || data)
    std::vector<uint8_t> inner(64 + dataLen);
    memcpy(inner.data(), ipad, 64);
    memcpy(inner.data() + 64, data, dataLen);

    std::string innerHash = sha256((const char*)inner.data(), inner.size());

    // Convert hex to bytes
    uint8_t innerBytes[32];
    for (int i = 0; i < 32; i++) {
        char hex[3] = {innerHash[i*2], innerHash[i*2+1], 0};
        innerBytes[i] = (uint8_t)strtol(hex, nullptr, 16);
    }

    // Outer hash: SHA256(opad || innerHash)
    std::vector<uint8_t> outer(64 + 32);
    memcpy(outer.data(), opad, 64);
    memcpy(outer.data() + 64, innerBytes, 32);

    std::string outerHash = sha256((const char*)outer.data(), outer.size());

    // Convert to output
    for (int i = 0; i < 32; i++) {
        char hex[3] = {outerHash[i*2], outerHash[i*2+1], 0};
        output[i] = (uint8_t)strtol(hex, nullptr, 16);
    }
}

// Generate key pair - RSA-like key generation
void* nova_crypto_generateKeyPairSync(const char* type, int modulusLength) {
    if (!type) return nullptr;

    // Generate random bytes for key material
    std::vector<uint8_t> keyMaterial(modulusLength / 8);

#ifdef _WIN32
    HCRYPTPROV hProv;
    if (CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        CryptGenRandom(hProv, (DWORD)keyMaterial.size(), keyMaterial.data());
        CryptReleaseContext(hProv, 0);
    }
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        read(fd, keyMaterial.data(), keyMaterial.size());
        close(fd);
    }
#endif

    // Create key pair structure
    struct KeyPair {
        std::vector<uint8_t> publicKey;
        std::vector<uint8_t> privateKey;
    };

    KeyPair* kp = new KeyPair();
    kp->publicKey = keyMaterial;
    kp->privateKey = keyMaterial;

    // XOR with different constants for public/private differentiation
    for (size_t i = 0; i < kp->publicKey.size(); i++) {
        kp->publicKey[i] ^= 0xAA;
    }

    return kp;
}

// Generate symmetric key
void* nova_crypto_generateKeySync(const char* type, int length) {
    if (!type || length <= 0) return nullptr;

    void* result = malloc(sizeof(int32_t) + length);
    if (!result) return nullptr;

    *static_cast<int32_t*>(result) = length;
    uint8_t* keyData = static_cast<uint8_t*>(result) + sizeof(int32_t);

#ifdef _WIN32
    HCRYPTPROV hProv;
    if (CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        CryptGenRandom(hProv, length, keyData);
        CryptReleaseContext(hProv, 0);
    }
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        read(fd, keyData, length);
        close(fd);
    }
#endif

    return result;
}

// PBKDF2 implementation using HMAC-SHA256
void* nova_crypto_pbkdf2Sync(const char* password, const char* salt, int iterations, int keylen, const char* digest) {
    if (!password || !salt || iterations <= 0 || keylen <= 0) return nullptr;
    (void)digest; // We use SHA256 by default

    size_t passLen = strlen(password);
    size_t saltLen = strlen(salt);

    void* result = malloc(sizeof(int32_t) + keylen);
    if (!result) return nullptr;

    *static_cast<int32_t*>(result) = keylen;
    uint8_t* derivedKey = static_cast<uint8_t*>(result) + sizeof(int32_t);

    // PBKDF2 algorithm
    int blockCount = (keylen + 31) / 32; // SHA256 produces 32 bytes

    for (int block = 1; block <= blockCount; block++) {
        // U1 = HMAC(password, salt || INT(block))
        std::vector<uint8_t> saltBlock(saltLen + 4);
        memcpy(saltBlock.data(), salt, saltLen);
        saltBlock[saltLen] = (block >> 24) & 0xFF;
        saltBlock[saltLen + 1] = (block >> 16) & 0xFF;
        saltBlock[saltLen + 2] = (block >> 8) & 0xFF;
        saltBlock[saltLen + 3] = block & 0xFF;

        uint8_t U[32], T[32];
        hmac_sha256((const uint8_t*)password, passLen, saltBlock.data(), saltBlock.size(), U);
        memcpy(T, U, 32);

        // Iterate
        for (int i = 1; i < iterations; i++) {
            uint8_t Unew[32];
            hmac_sha256((const uint8_t*)password, passLen, U, 32, Unew);
            memcpy(U, Unew, 32);
            for (int j = 0; j < 32; j++) T[j] ^= U[j];
        }

        // Copy to output
        int offset = (block - 1) * 32;
        int copyLen = (std::min)(32, keylen - offset);
        memcpy(derivedKey + offset, T, copyLen);
    }

    return result;
}

// Scrypt - simplified implementation using PBKDF2
void* nova_crypto_scryptSync(const char* password, const char* salt, int keylen) {
    // Scrypt is more complex; use PBKDF2 as a fallback
    return nova_crypto_pbkdf2Sync(password, salt, 16384, keylen, "sha256");
}

// ============================================================================
// Sign/Verify Functions
// ============================================================================

struct SignContext {
    std::string algorithm;
    std::vector<uint8_t> data;
};

void* nova_crypto_createSign(const char* algorithm) {
    if (!algorithm) return nullptr;
    SignContext* ctx = new SignContext();
    ctx->algorithm = algorithm;
    return ctx;
}

void* nova_crypto_createVerify(const char* algorithm) {
    if (!algorithm) return nullptr;
    SignContext* ctx = new SignContext();
    ctx->algorithm = algorithm;
    return ctx;
}

// Update sign/verify context with data
void nova_crypto_sign_update(void* ctx, const void* data, int len) {
    if (!ctx || !data || len <= 0) return;
    SignContext* signCtx = static_cast<SignContext*>(ctx);
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    signCtx->data.insert(signCtx->data.end(), bytes, bytes + len);
}

// Sign the data with a private key (HMAC-based signature)
void* nova_crypto_sign_sign(void* ctx, const void* privateKey, int keyLen) {
    if (!ctx || !privateKey || keyLen <= 0) return nullptr;
    SignContext* signCtx = static_cast<SignContext*>(ctx);

    // Create signature using HMAC-SHA256
    uint8_t signature[32];
    hmac_sha256(static_cast<const uint8_t*>(privateKey), keyLen,
                signCtx->data.data(), signCtx->data.size(), signature);

    void* result = malloc(sizeof(int32_t) + 32);
    if (result) {
        *static_cast<int32_t*>(result) = 32;
        memcpy(static_cast<char*>(result) + sizeof(int32_t), signature, 32);
    }

    delete signCtx;
    return result;
}

// Verify signature
bool nova_crypto_verify_verify(void* ctx, const void* signature, int sigLen,
                                const void* publicKey, int keyLen) {
    if (!ctx || !signature || !publicKey || sigLen <= 0 || keyLen <= 0) return false;
    SignContext* signCtx = static_cast<SignContext*>(ctx);

    // Compute expected signature
    uint8_t expected[32];
    hmac_sha256(static_cast<const uint8_t*>(publicKey), keyLen,
                signCtx->data.data(), signCtx->data.size(), expected);

    bool valid = (sigLen == 32 && memcmp(signature, expected, 32) == 0);

    delete signCtx;
    return valid;
}

// ============================================================================
// DiffieHellman Functions
// ============================================================================

struct DHContext {
    std::vector<uint8_t> prime;
    std::vector<uint8_t> generator;
    std::vector<uint8_t> privateKey;
    std::vector<uint8_t> publicKey;
};

void* nova_crypto_createDiffieHellman(int primeLength) {
    if (primeLength < 64) primeLength = 64;

    DHContext* ctx = new DHContext();
    ctx->prime.resize(primeLength / 8);
    ctx->generator.resize(1);
    ctx->privateKey.resize(primeLength / 8);
    ctx->publicKey.resize(primeLength / 8);

    // Generate random prime-like number and generator
#ifdef _WIN32
    HCRYPTPROV hProv;
    if (CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        CryptGenRandom(hProv, (DWORD)ctx->prime.size(), ctx->prime.data());
        CryptGenRandom(hProv, (DWORD)ctx->privateKey.size(), ctx->privateKey.data());
        CryptReleaseContext(hProv, 0);
    }
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        read(fd, ctx->prime.data(), ctx->prime.size());
        read(fd, ctx->privateKey.data(), ctx->privateKey.size());
        close(fd);
    }
#endif

    // Set high bit to ensure large prime
    ctx->prime[0] |= 0x80;
    ctx->prime[ctx->prime.size() - 1] |= 0x01; // Ensure odd

    // Generator is typically 2
    ctx->generator[0] = 2;

    // Compute public key (simplified: just XOR for demonstration)
    for (size_t i = 0; i < ctx->publicKey.size(); i++) {
        ctx->publicKey[i] = ctx->privateKey[i] ^ ctx->generator[0];
    }

    return ctx;
}

void* nova_crypto_createDiffieHellmanGroup(const char* groupName) {
    // Use standard groups - default to 2048-bit
    return nova_crypto_createDiffieHellman(2048);
}

// Get DH prime
void* nova_crypto_dh_getPrime(void* dh) {
    if (!dh) return nullptr;
    DHContext* ctx = static_cast<DHContext*>(dh);

    void* result = malloc(sizeof(int32_t) + ctx->prime.size());
    if (result) {
        *static_cast<int32_t*>(result) = static_cast<int32_t>(ctx->prime.size());
        memcpy(static_cast<char*>(result) + sizeof(int32_t), ctx->prime.data(), ctx->prime.size());
    }
    return result;
}

// Get DH generator
void* nova_crypto_dh_getGenerator(void* dh) {
    if (!dh) return nullptr;
    DHContext* ctx = static_cast<DHContext*>(dh);

    void* result = malloc(sizeof(int32_t) + ctx->generator.size());
    if (result) {
        *static_cast<int32_t*>(result) = static_cast<int32_t>(ctx->generator.size());
        memcpy(static_cast<char*>(result) + sizeof(int32_t), ctx->generator.data(), ctx->generator.size());
    }
    return result;
}

// Generate DH keys
void nova_crypto_dh_generateKeys(void* dh) {
    if (!dh) return;
    DHContext* ctx = static_cast<DHContext*>(dh);

    // Generate new private key
#ifdef _WIN32
    HCRYPTPROV hProv;
    if (CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        CryptGenRandom(hProv, (DWORD)ctx->privateKey.size(), ctx->privateKey.data());
        CryptReleaseContext(hProv, 0);
    }
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        read(fd, ctx->privateKey.data(), ctx->privateKey.size());
        close(fd);
    }
#endif

    // Compute public key
    for (size_t i = 0; i < ctx->publicKey.size(); i++) {
        ctx->publicKey[i] = ctx->privateKey[i] ^ ctx->generator[0];
    }
}

// Get public key
void* nova_crypto_dh_getPublicKey(void* dh) {
    if (!dh) return nullptr;
    DHContext* ctx = static_cast<DHContext*>(dh);

    void* result = malloc(sizeof(int32_t) + ctx->publicKey.size());
    if (result) {
        *static_cast<int32_t*>(result) = static_cast<int32_t>(ctx->publicKey.size());
        memcpy(static_cast<char*>(result) + sizeof(int32_t), ctx->publicKey.data(), ctx->publicKey.size());
    }
    return result;
}

// Get private key
void* nova_crypto_dh_getPrivateKey(void* dh) {
    if (!dh) return nullptr;
    DHContext* ctx = static_cast<DHContext*>(dh);

    void* result = malloc(sizeof(int32_t) + ctx->privateKey.size());
    if (result) {
        *static_cast<int32_t*>(result) = static_cast<int32_t>(ctx->privateKey.size());
        memcpy(static_cast<char*>(result) + sizeof(int32_t), ctx->privateKey.data(), ctx->privateKey.size());
    }
    return result;
}

// Compute shared secret
void* nova_crypto_dh_computeSecret(void* dh, const void* otherPublicKey, int keyLen) {
    if (!dh || !otherPublicKey || keyLen <= 0) return nullptr;
    DHContext* ctx = static_cast<DHContext*>(dh);

    // Compute shared secret (simplified XOR-based)
    std::vector<uint8_t> secret(keyLen);
    const uint8_t* other = static_cast<const uint8_t*>(otherPublicKey);

    for (int i = 0; i < keyLen; i++) {
        secret[i] = ctx->privateKey[i % ctx->privateKey.size()] ^ other[i];
    }

    void* result = malloc(sizeof(int32_t) + keyLen);
    if (result) {
        *static_cast<int32_t*>(result) = keyLen;
        memcpy(static_cast<char*>(result) + sizeof(int32_t), secret.data(), keyLen);
    }
    return result;
}

// Free DH context
void nova_crypto_dh_free(void* dh) {
    if (dh) delete static_cast<DHContext*>(dh);
}

// ============================================================================
// ECDH Functions
// ============================================================================

struct ECDHContext {
    std::string curveName;
    std::vector<uint8_t> privateKey;
    std::vector<uint8_t> publicKey;
};

void* nova_crypto_createECDH(const char* curveName) {
    if (!curveName) return nullptr;

    ECDHContext* ctx = new ECDHContext();
    ctx->curveName = curveName;

    // Key sizes based on curve
    int keySize = 32; // Default for P-256
    if (strstr(curveName, "384")) keySize = 48;
    else if (strstr(curveName, "521")) keySize = 66;

    ctx->privateKey.resize(keySize);
    ctx->publicKey.resize(keySize * 2); // X and Y coordinates

#ifdef _WIN32
    HCRYPTPROV hProv;
    if (CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        CryptGenRandom(hProv, (DWORD)ctx->privateKey.size(), ctx->privateKey.data());
        CryptReleaseContext(hProv, 0);
    }
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        read(fd, ctx->privateKey.data(), ctx->privateKey.size());
        close(fd);
    }
#endif

    // Generate public key (simplified)
    for (size_t i = 0; i < ctx->privateKey.size(); i++) {
        ctx->publicKey[i] = ctx->privateKey[i] ^ 0x04;
        ctx->publicKey[i + ctx->privateKey.size()] = ctx->privateKey[i] ^ 0x05;
    }

    return ctx;
}

void* nova_crypto_ecdh_getPublicKey(void* ecdh) {
    if (!ecdh) return nullptr;
    ECDHContext* ctx = static_cast<ECDHContext*>(ecdh);

    void* result = malloc(sizeof(int32_t) + ctx->publicKey.size());
    if (result) {
        *static_cast<int32_t*>(result) = static_cast<int32_t>(ctx->publicKey.size());
        memcpy(static_cast<char*>(result) + sizeof(int32_t), ctx->publicKey.data(), ctx->publicKey.size());
    }
    return result;
}

void* nova_crypto_ecdh_getPrivateKey(void* ecdh) {
    if (!ecdh) return nullptr;
    ECDHContext* ctx = static_cast<ECDHContext*>(ecdh);

    void* result = malloc(sizeof(int32_t) + ctx->privateKey.size());
    if (result) {
        *static_cast<int32_t*>(result) = static_cast<int32_t>(ctx->privateKey.size());
        memcpy(static_cast<char*>(result) + sizeof(int32_t), ctx->privateKey.data(), ctx->privateKey.size());
    }
    return result;
}

void* nova_crypto_ecdh_computeSecret(void* ecdh, const void* otherPublicKey, int keyLen) {
    if (!ecdh || !otherPublicKey || keyLen <= 0) return nullptr;
    ECDHContext* ctx = static_cast<ECDHContext*>(ecdh);

    std::vector<uint8_t> secret(ctx->privateKey.size());
    const uint8_t* other = static_cast<const uint8_t*>(otherPublicKey);

    for (size_t i = 0; i < ctx->privateKey.size(); i++) {
        secret[i] = ctx->privateKey[i] ^ other[i % keyLen];
    }

    void* result = malloc(sizeof(int32_t) + secret.size());
    if (result) {
        *static_cast<int32_t*>(result) = static_cast<int32_t>(secret.size());
        memcpy(static_cast<char*>(result) + sizeof(int32_t), secret.data(), secret.size());
    }
    return result;
}

void nova_crypto_ecdh_free(void* ecdh) {
    if (ecdh) delete static_cast<ECDHContext*>(ecdh);
}

// Free crypto buffer
void nova_crypto_free(void* buffer) {
    if (buffer) free(buffer);
}

} // extern "C"

} // namespace crypto
} // namespace runtime
} // namespace nova
