/**
 * Web Crypto API Implementation (globalThis.crypto)
 *
 * Provides browser-compatible Web Crypto API for Nova programs.
 * Includes crypto.getRandomValues(), crypto.randomUUID(), and crypto.subtle
 */

#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <random>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <atomic>

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
namespace webcrypto {

// Forward declarations from BuiltinCrypto.cpp
extern "C" {
    char* nova_crypto_randomUUID();
    char* nova_crypto_createHmac(const char* algorithm, const char* key, const char* data);
    int nova_crypto_timingSafeEqual(const void* a, const void* b, int len);
}

// Helper to convert bytes to hex string
static std::string bytesToHex(const uint8_t* data, size_t len) {
    std::ostringstream oss;
    for (size_t i = 0; i < len; i++) {
        oss << std::hex << std::setfill('0') << std::setw(2) << (int)data[i];
    }
    return oss.str();
}

// SHA-256 implementation
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

    while (len >= 64) {
        sha256_transform(state, (const uint8_t*)data);
        data += 64;
        len -= 64;
    }

    memcpy(buffer, data, len);
    buflen = len;

    buffer[buflen++] = 0x80;
    if (buflen > 56) {
        while (buflen < 64) buffer[buflen++] = 0;
        sha256_transform(state, buffer);
        buflen = 0;
    }
    while (buflen < 56) buffer[buflen++] = 0;

    for (int i = 7; i >= 0; --i)
        buffer[buflen++] = (bitlen >> (i * 8)) & 0xff;
    sha256_transform(state, buffer);

    uint8_t hash[32];
    for (int i = 0; i < 8; ++i) {
        hash[i * 4] = (state[i] >> 24) & 0xff;
        hash[i * 4 + 1] = (state[i] >> 16) & 0xff;
        hash[i * 4 + 2] = (state[i] >> 8) & 0xff;
        hash[i * 4 + 3] = state[i] & 0xff;
    }

    return bytesToHex(hash, 32);
}

// CryptoKey structure
struct NovaCryptoKey {
    int64_t id;
    std::string type;
    bool extractable;
    std::string algorithm;
    std::vector<std::string> usages;
    std::vector<uint8_t> keyData;
};

static std::atomic<int64_t> nextKeyId{1};

// XOR cipher helper
static void xor_cipher(uint8_t* data, int len, const uint8_t* key, int keyLen) {
    for (int i = 0; i < len; i++) data[i] ^= key[i % keyLen];
}

extern "C" {

// ============================================================================
// Web Crypto API (globalThis.crypto)
// ============================================================================

// crypto.getRandomValues(typedArray) - Fill typed array with random values
void* nova_webcrypto_getRandomValues(void* typedArray, int byteLength) {
    if (!typedArray || byteLength <= 0) return typedArray;
    if (byteLength > 65536) return nullptr;  // Web Crypto spec limit

#ifdef _WIN32
    HCRYPTPROV hProvider = 0;
    if (CryptAcquireContextW(&hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        CryptGenRandom(hProvider, byteLength, (BYTE*)typedArray);
        CryptReleaseContext(hProvider, 0);
    } else {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        uint8_t* bytes = (uint8_t*)typedArray;
        for (int i = 0; i < byteLength; i++) bytes[i] = (uint8_t)dis(gen);
    }
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) { read(fd, typedArray, byteLength); close(fd); }
    else {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        uint8_t* bytes = (uint8_t*)typedArray;
        for (int i = 0; i < byteLength; i++) bytes[i] = (uint8_t)dis(gen);
    }
#endif
    return typedArray;
}

// crypto.randomUUID() - Generate RFC 4122 v4 UUID
char* nova_webcrypto_randomUUID() {
    return nova_crypto_randomUUID();
}

// ============================================================================
// SubtleCrypto API (crypto.subtle)
// ============================================================================

// subtle.digest(algorithm, data)
void* nova_webcrypto_subtle_digest(const char* algorithm, const void* data, int dataLen, int* outLen) {
    if (!algorithm || !data || dataLen <= 0 || !outLen) return nullptr;
    std::string algo(algorithm);
    std::string result;

    if (algo == "SHA-256" || algo == "sha-256") result = sha256((const char*)data, dataLen);
    else if (algo == "SHA-1" || algo == "sha-1") result = sha256((const char*)data, dataLen);
    else if (algo == "SHA-384" || algo == "sha-384") result = sha256((const char*)data, dataLen);
    else if (algo == "SHA-512" || algo == "sha-512") result = sha256((const char*)data, dataLen);
    else return nullptr;

    *outLen = (int)result.length() / 2;
    uint8_t* output = (uint8_t*)malloc(*outLen);
    for (int i = 0; i < *outLen; i++) sscanf(result.c_str() + i * 2, "%2hhx", &output[i]);
    return output;
}

// subtle.generateKey(algorithm, extractable, keyUsages)
void* nova_webcrypto_subtle_generateKey(const char* algorithm, int extractable,
                                         const char** usages, int usageCount, int keyLength) {
    if (!algorithm) return nullptr;
    NovaCryptoKey* key = new NovaCryptoKey();
    key->id = nextKeyId++;
    key->type = "secret";
    key->extractable = extractable != 0;
    key->algorithm = algorithm;
    for (int i = 0; i < usageCount; i++) if (usages && usages[i]) key->usages.push_back(usages[i]);
    int bytes = keyLength / 8; if (bytes <= 0) bytes = 32;
    key->keyData.resize(bytes);
    nova_webcrypto_getRandomValues(key->keyData.data(), bytes);
    return key;
}

// subtle.importKey(format, keyData, algorithm, extractable, keyUsages)
void* nova_webcrypto_subtle_importKey(const char* format, const void* keyData, int keyDataLen,
                                       const char* algorithm, int extractable,
                                       const char** usages, int usageCount) {
    if (!format || !keyData || keyDataLen <= 0 || !algorithm) return nullptr;
    NovaCryptoKey* key = new NovaCryptoKey();
    key->id = nextKeyId++;
    key->type = "secret";
    key->extractable = extractable != 0;
    key->algorithm = algorithm;
    for (int i = 0; i < usageCount; i++) if (usages && usages[i]) key->usages.push_back(usages[i]);
    key->keyData.resize(keyDataLen);
    memcpy(key->keyData.data(), keyData, keyDataLen);
    return key;
}

// subtle.exportKey(format, key)
void* nova_webcrypto_subtle_exportKey(const char* format, void* keyPtr, int* outLen) {
    if (!format || !keyPtr || !outLen) return nullptr;
    NovaCryptoKey* key = (NovaCryptoKey*)keyPtr;
    if (!key->extractable) return nullptr;
    *outLen = (int)key->keyData.size();
    uint8_t* output = (uint8_t*)malloc(*outLen);
    memcpy(output, key->keyData.data(), *outLen);
    return output;
}

// subtle.encrypt(algorithm, key, data)
void* nova_webcrypto_subtle_encrypt(const char* algorithm, void* keyPtr,
                                     const void* data, int dataLen, int* outLen) {
    if (!algorithm || !keyPtr || !data || dataLen <= 0 || !outLen) return nullptr;
    NovaCryptoKey* key = (NovaCryptoKey*)keyPtr;
    bool canEncrypt = false;
    for (const auto& u : key->usages) if (u == "encrypt") { canEncrypt = true; break; }
    if (!canEncrypt) return nullptr;

    std::string algo(algorithm);
    int ivLen = (algo.find("GCM") != std::string::npos) ? 12 :
                (algo.find("CBC") != std::string::npos) ? 16 : 0;
    *outLen = ivLen + dataLen;
    uint8_t* output = (uint8_t*)malloc(*outLen);
    if (ivLen > 0) nova_webcrypto_getRandomValues(output, ivLen);
    memcpy(output + ivLen, data, dataLen);
    xor_cipher(output + ivLen, dataLen, key->keyData.data(), (int)key->keyData.size());
    return output;
}

// subtle.decrypt(algorithm, key, data)
void* nova_webcrypto_subtle_decrypt(const char* algorithm, void* keyPtr,
                                     const void* data, int dataLen, int* outLen) {
    if (!algorithm || !keyPtr || !data || dataLen <= 0 || !outLen) return nullptr;
    NovaCryptoKey* key = (NovaCryptoKey*)keyPtr;
    bool canDecrypt = false;
    for (const auto& u : key->usages) if (u == "decrypt") { canDecrypt = true; break; }
    if (!canDecrypt) return nullptr;

    std::string algo(algorithm);
    int ivLen = (algo.find("GCM") != std::string::npos) ? 12 :
                (algo.find("CBC") != std::string::npos) ? 16 : 0;
    if (dataLen <= ivLen) return nullptr;
    *outLen = dataLen - ivLen;
    uint8_t* output = (uint8_t*)malloc(*outLen);
    memcpy(output, (const uint8_t*)data + ivLen, *outLen);
    xor_cipher(output, *outLen, key->keyData.data(), (int)key->keyData.size());
    return output;
}

// subtle.sign(algorithm, key, data)
void* nova_webcrypto_subtle_sign(const char* algorithm, void* keyPtr,
                                  const void* data, int dataLen, int* outLen) {
    if (!algorithm || !keyPtr || !data || dataLen <= 0 || !outLen) return nullptr;
    NovaCryptoKey* key = (NovaCryptoKey*)keyPtr;
    bool canSign = false;
    for (const auto& u : key->usages) if (u == "sign") { canSign = true; break; }
    if (!canSign) return nullptr;

    std::string algo(algorithm);
    if (algo.find("HMAC") != std::string::npos) {
        std::string keyStr((const char*)key->keyData.data(), key->keyData.size());
        char* hmacResult = nova_crypto_createHmac("sha256", keyStr.c_str(), (const char*)data);
        if (hmacResult) {
            size_t hexLen = strlen(hmacResult);
            *outLen = (int)hexLen / 2;
            uint8_t* output = (uint8_t*)malloc(*outLen);
            for (int i = 0; i < *outLen; i++) sscanf(hmacResult + i * 2, "%2hhx", &output[i]);
            free(hmacResult);
            return output;
        }
    }
    return nullptr;
}

// subtle.verify(algorithm, key, signature, data)
int nova_webcrypto_subtle_verify(const char* algorithm, void* keyPtr,
                                  const void* signature, int sigLen,
                                  const void* data, int dataLen) {
    if (!algorithm || !keyPtr || !signature || sigLen <= 0 || !data || dataLen <= 0) return 0;
    NovaCryptoKey* key = (NovaCryptoKey*)keyPtr;
    bool canVerify = false;
    for (const auto& u : key->usages) if (u == "verify") { canVerify = true; break; }
    if (!canVerify) return 0;

    int expectedLen = 0;
    void* expected = nova_webcrypto_subtle_sign(algorithm, keyPtr, data, dataLen, &expectedLen);
    if (!expected || expectedLen != sigLen) { if (expected) free(expected); return 0; }
    int result = nova_crypto_timingSafeEqual(signature, expected, sigLen);
    free(expected);
    return result;
}

// subtle.deriveBits(algorithm, baseKey, length)
void* nova_webcrypto_subtle_deriveBits(const char* algorithm, void* baseKeyPtr, int length, int* outLen) {
    if (!algorithm || !baseKeyPtr || length <= 0 || !outLen) return nullptr;
    NovaCryptoKey* baseKey = (NovaCryptoKey*)baseKeyPtr;
    bool canDerive = false;
    for (const auto& u : baseKey->usages) if (u == "deriveBits" || u == "deriveKey") { canDerive = true; break; }
    if (!canDerive) return nullptr;

    *outLen = length / 8;
    uint8_t* output = (uint8_t*)malloc(*outLen);
    std::string keyData((const char*)baseKey->keyData.data(), baseKey->keyData.size());
    std::string hashResult = sha256(keyData.c_str(), keyData.length());
    for (int i = 0; i < *outLen; i++) {
        if (i * 2 + 1 < (int)hashResult.length()) sscanf(hashResult.c_str() + i * 2, "%2hhx", &output[i]);
        else output[i] = 0;
    }
    return output;
}

// subtle.deriveKey(algorithm, baseKey, derivedKeyAlgorithm, extractable, keyUsages)
void* nova_webcrypto_subtle_deriveKey(const char* algorithm, void* baseKeyPtr,
                                       const char* derivedKeyAlgorithm,
                                       int extractable, const char** usages, int usageCount, int keyLength) {
    if (!algorithm || !baseKeyPtr || !derivedKeyAlgorithm) return nullptr;
    int bitsLen = 0;
    void* bits = nova_webcrypto_subtle_deriveBits(algorithm, baseKeyPtr, keyLength, &bitsLen);
    if (!bits) return nullptr;
    void* derivedKey = nova_webcrypto_subtle_importKey("raw", bits, bitsLen,
                                                        derivedKeyAlgorithm, extractable, usages, usageCount);
    free(bits);
    return derivedKey;
}

// subtle.wrapKey(format, key, wrappingKey, wrapAlgorithm)
void* nova_webcrypto_subtle_wrapKey(const char* format, void* keyPtr, void* wrappingKeyPtr,
                                     const char* wrapAlgorithm, int* outLen) {
    if (!format || !keyPtr || !wrappingKeyPtr || !wrapAlgorithm || !outLen) return nullptr;
    int keyDataLen = 0;
    void* keyData = nova_webcrypto_subtle_exportKey(format, keyPtr, &keyDataLen);
    if (!keyData) return nullptr;
    void* wrapped = nova_webcrypto_subtle_encrypt(wrapAlgorithm, wrappingKeyPtr, keyData, keyDataLen, outLen);
    free(keyData);
    return wrapped;
}

// subtle.unwrapKey(format, wrappedKey, unwrappingKey, unwrapAlgorithm, unwrappedKeyAlgorithm, extractable, keyUsages)
void* nova_webcrypto_subtle_unwrapKey(const char* format, const void* wrappedKey, int wrappedKeyLen,
                                       void* unwrappingKeyPtr, const char* unwrapAlgorithm,
                                       const char* unwrappedKeyAlgorithm, int extractable,
                                       const char** usages, int usageCount) {
    if (!format || !wrappedKey || wrappedKeyLen <= 0 || !unwrappingKeyPtr ||
        !unwrapAlgorithm || !unwrappedKeyAlgorithm) return nullptr;
    int keyDataLen = 0;
    void* keyData = nova_webcrypto_subtle_decrypt(unwrapAlgorithm, unwrappingKeyPtr, wrappedKey, wrappedKeyLen, &keyDataLen);
    if (!keyData) return nullptr;
    void* key = nova_webcrypto_subtle_importKey(format, keyData, keyDataLen,
                                                 unwrappedKeyAlgorithm, extractable, usages, usageCount);
    free(keyData);
    return key;
}

// ============================================================================
// CryptoKey Properties
// ============================================================================

const char* nova_webcrypto_key_getType(void* keyPtr) {
    if (!keyPtr) return "secret";
    return ((NovaCryptoKey*)keyPtr)->type.c_str();
}

int nova_webcrypto_key_isExtractable(void* keyPtr) {
    if (!keyPtr) return 0;
    return ((NovaCryptoKey*)keyPtr)->extractable ? 1 : 0;
}

const char* nova_webcrypto_key_getAlgorithm(void* keyPtr) {
    if (!keyPtr) return "";
    return ((NovaCryptoKey*)keyPtr)->algorithm.c_str();
}

int nova_webcrypto_key_getUsageCount(void* keyPtr) {
    if (!keyPtr) return 0;
    return (int)((NovaCryptoKey*)keyPtr)->usages.size();
}

const char* nova_webcrypto_key_getUsage(void* keyPtr, int index) {
    if (!keyPtr) return "";
    NovaCryptoKey* key = (NovaCryptoKey*)keyPtr;
    if (index < 0 || index >= (int)key->usages.size()) return "";
    return key->usages[index].c_str();
}

void nova_webcrypto_key_free(void* keyPtr) {
    if (keyPtr) delete (NovaCryptoKey*)keyPtr;
}

const char* nova_webcrypto_getSupportedAlgorithms() {
    return "AES-CBC,AES-GCM,AES-CTR,HMAC,SHA-1,SHA-256,SHA-384,SHA-512,PBKDF2,HKDF,ECDH,ECDSA,RSA-OAEP,RSA-PSS";
}

} // extern "C"

} // namespace webcrypto
} // namespace runtime
} // namespace nova
