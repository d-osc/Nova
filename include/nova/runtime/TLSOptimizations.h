/**
 * Nova TLS Optimizations Header
 *
 * Extreme performance optimizations for HTTPS/TLS connections
 * Target: 13x faster than Node.js HTTPS
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <memory>
#include <chrono>

// Platform-specific includes
#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

// Hardware acceleration detection
#ifdef __AES__
#define HAS_AES_NI 1
#include <wmmintrin.h>  // AES-NI intrinsics
#endif

#ifdef __AVX2__
#define HAS_AVX2 1
#include <immintrin.h>
#endif

namespace nova::tls {

// ============================================================================
// OPTIMIZATION 1: Compiler Hints and Macros
// ============================================================================

// Branch prediction (same as HTTP/2)
#ifdef __GNUC__
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#endif

// Function attributes
#ifdef __GNUC__
#define HOT_FUNCTION __attribute__((hot))
#define COLD_FUNCTION __attribute__((cold))
#define PURE_FUNCTION __attribute__((pure))
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define HOT_FUNCTION
#define COLD_FUNCTION
#define PURE_FUNCTION
#define ALWAYS_INLINE __forceinline
#else
#define HOT_FUNCTION
#define COLD_FUNCTION
#define PURE_FUNCTION
#define ALWAYS_INLINE inline
#endif

// Cache line alignment
#define CACHE_LINE_SIZE 64
#ifdef __GNUC__
#define CACHE_ALIGNED __attribute__((aligned(CACHE_LINE_SIZE)))
#elif defined(_MSC_VER)
#define CACHE_ALIGNED __declspec(align(CACHE_LINE_SIZE))
#else
#define CACHE_ALIGNED
#endif

// Prefetch hints
#ifdef __GNUC__
#define PREFETCH_READ(addr) __builtin_prefetch(addr, 0, 3)
#define PREFETCH_WRITE(addr) __builtin_prefetch(addr, 1, 3)
#else
#define PREFETCH_READ(addr) ((void)0)
#define PREFETCH_WRITE(addr) ((void)0)
#endif

// ============================================================================
// OPTIMIZATION 2: AES-NI Hardware Acceleration
// ============================================================================

#ifdef HAS_AES_NI

// AES-128 encryption using AES-NI (10 rounds)
HOT_FUNCTION ALWAYS_INLINE
void aes128_encrypt_block_ni(const uint8_t* plaintext, uint8_t* ciphertext,
                              const __m128i* round_keys) {
    __m128i block = _mm_loadu_si128((const __m128i*)plaintext);

    // Initial round
    block = _mm_xor_si128(block, round_keys[0]);

    // 9 main rounds (unrolled for performance)
    block = _mm_aesenc_si128(block, round_keys[1]);
    block = _mm_aesenc_si128(block, round_keys[2]);
    block = _mm_aesenc_si128(block, round_keys[3]);
    block = _mm_aesenc_si128(block, round_keys[4]);
    block = _mm_aesenc_si128(block, round_keys[5]);
    block = _mm_aesenc_si128(block, round_keys[6]);
    block = _mm_aesenc_si128(block, round_keys[7]);
    block = _mm_aesenc_si128(block, round_keys[8]);
    block = _mm_aesenc_si128(block, round_keys[9]);

    // Final round
    block = _mm_aesenclast_si128(block, round_keys[10]);

    _mm_storeu_si128((__m128i*)ciphertext, block);
}

// AES-256 encryption using AES-NI (14 rounds)
HOT_FUNCTION ALWAYS_INLINE
void aes256_encrypt_block_ni(const uint8_t* plaintext, uint8_t* ciphertext,
                              const __m128i* round_keys) {
    __m128i block = _mm_loadu_si128((const __m128i*)plaintext);

    // Initial round
    block = _mm_xor_si128(block, round_keys[0]);

    // 13 main rounds (unrolled)
    for (int i = 1; i < 14; i++) {
        block = _mm_aesenc_si128(block, round_keys[i]);
    }

    // Final round
    block = _mm_aesenclast_si128(block, round_keys[14]);

    _mm_storeu_si128((__m128i*)ciphertext, block);
}

#endif // HAS_AES_NI

// ============================================================================
// OPTIMIZATION 3: SIMD-Parallelized AES-GCM
// ============================================================================

#ifdef HAS_AVX2

// Process 4 AES blocks in parallel using AVX2
HOT_FUNCTION ALWAYS_INLINE
void aes_gcm_encrypt_4blocks_avx2(const uint8_t* plaintext, uint8_t* ciphertext,
                                   const __m128i* round_keys, size_t num_rounds) {
    // Load 4 blocks (64 bytes)
    __m128i block0 = _mm_loadu_si128((const __m128i*)(plaintext + 0));
    __m128i block1 = _mm_loadu_si128((const __m128i*)(plaintext + 16));
    __m128i block2 = _mm_loadu_si128((const __m128i*)(plaintext + 32));
    __m128i block3 = _mm_loadu_si128((const __m128i*)(plaintext + 48));

    // Initial XOR with round key 0
    block0 = _mm_xor_si128(block0, round_keys[0]);
    block1 = _mm_xor_si128(block1, round_keys[0]);
    block2 = _mm_xor_si128(block2, round_keys[0]);
    block3 = _mm_xor_si128(block3, round_keys[0]);

    // Main rounds (parallel)
    for (size_t i = 1; i < num_rounds; i++) {
        block0 = _mm_aesenc_si128(block0, round_keys[i]);
        block1 = _mm_aesenc_si128(block1, round_keys[i]);
        block2 = _mm_aesenc_si128(block2, round_keys[i]);
        block3 = _mm_aesenc_si128(block3, round_keys[i]);
    }

    // Final round
    block0 = _mm_aesenclast_si128(block0, round_keys[num_rounds]);
    block1 = _mm_aesenclast_si128(block1, round_keys[num_rounds]);
    block2 = _mm_aesenclast_si128(block2, round_keys[num_rounds]);
    block3 = _mm_aesenclast_si128(block3, round_keys[num_rounds]);

    // Store results
    _mm_storeu_si128((__m128i*)(ciphertext + 0), block0);
    _mm_storeu_si128((__m128i*)(ciphertext + 16), block1);
    _mm_storeu_si128((__m128i*)(ciphertext + 32), block2);
    _mm_storeu_si128((__m128i*)(ciphertext + 48), block3);
}

#endif // HAS_AVX2

// ============================================================================
// OPTIMIZATION 4: TLS Session Cache with LRU
// ============================================================================

struct CACHE_ALIGNED TLSSession {
    uint8_t session_id[32];
    uint8_t master_secret[48];
    uint16_t cipher_suite;
    uint64_t timestamp;
    uint32_t use_count;

    TLSSession() : cipher_suite(0), timestamp(0), use_count(0) {
        memset(session_id, 0, sizeof(session_id));
        memset(master_secret, 0, sizeof(master_secret));
    }
};

class CACHE_ALIGNED TLSSessionCache {
private:
    static constexpr size_t CACHE_SIZE = 10000;
    static constexpr uint64_t SESSION_TIMEOUT = 7200; // 2 hours in seconds

    struct CacheEntry {
        TLSSession session;
        uint64_t last_access;
        bool valid;
    };

    CACHE_ALIGNED CacheEntry cache_[CACHE_SIZE];
    std::unordered_map<std::string, size_t> session_index_;

    // LRU tracking
    size_t lru_clock_;

public:
    TLSSessionCache() : lru_clock_(0) {
        for (size_t i = 0; i < CACHE_SIZE; i++) {
            cache_[i].valid = false;
            cache_[i].last_access = 0;
        }
    }

    // OPTIMIZATION: Fast session lookup with prefetching
    HOT_FUNCTION
    TLSSession* get(const uint8_t* session_id, size_t id_len) {
        std::string key((const char*)session_id, id_len);

        auto it = session_index_.find(key);
        if (UNLIKELY(it == session_index_.end())) {
            return nullptr;
        }

        size_t idx = it->second;
        PREFETCH_READ(&cache_[idx]);

        CacheEntry& entry = cache_[idx];

        // Check validity and timeout
        uint64_t now = get_timestamp();
        if (UNLIKELY(!entry.valid || (now - entry.session.timestamp) > SESSION_TIMEOUT)) {
            // Expired - remove from cache
            session_index_.erase(it);
            entry.valid = false;
            return nullptr;
        }

        // Update LRU
        entry.last_access = lru_clock_++;
        entry.session.use_count++;

        return &entry.session;
    }

    // OPTIMIZATION: Fast session insertion with LRU eviction
    HOT_FUNCTION
    void put(const uint8_t* session_id, size_t id_len, const TLSSession& session) {
        std::string key((const char*)session_id, id_len);

        // Check if already exists
        auto it = session_index_.find(key);
        if (it != session_index_.end()) {
            size_t idx = it->second;
            cache_[idx].session = session;
            cache_[idx].last_access = lru_clock_++;
            return;
        }

        // Find LRU entry to evict
        size_t lru_idx = 0;
        uint64_t min_access = UINT64_MAX;

        for (size_t i = 0; i < CACHE_SIZE; i++) {
            if (!cache_[i].valid) {
                lru_idx = i;
                break;
            }
            if (cache_[i].last_access < min_access) {
                min_access = cache_[i].last_access;
                lru_idx = i;
            }
        }

        // Evict old entry if needed
        if (cache_[lru_idx].valid) {
            std::string old_key((const char*)cache_[lru_idx].session.session_id, 32);
            session_index_.erase(old_key);
        }

        // Insert new entry
        cache_[lru_idx].session = session;
        cache_[lru_idx].valid = true;
        cache_[lru_idx].last_access = lru_clock_++;
        session_index_[key] = lru_idx;
    }

private:
    ALWAYS_INLINE uint64_t get_timestamp() const {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
    }
};

// ============================================================================
// OPTIMIZATION 5: Zero-Copy TLS Buffers
// ============================================================================

struct CACHE_ALIGNED TLSBuffer {
    uint8_t* data;
    size_t capacity;
    size_t length;
    bool owned;

    TLSBuffer() : data(nullptr), capacity(0), length(0), owned(false) {}

    ~TLSBuffer() {
        if (owned && data) {
            free(data);
        }
    }

    // OPTIMIZATION: Allocate aligned buffer for SIMD operations
    HOT_FUNCTION
    void allocate(size_t size) {
        if (owned && data) {
            free(data);
        }

        // Align to cache line for better performance
        size_t aligned_size = (size + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1);

#ifdef _WIN32
        data = (uint8_t*)_aligned_malloc(aligned_size, CACHE_LINE_SIZE);
#else
        posix_memalign((void**)&data, CACHE_LINE_SIZE, aligned_size);
#endif

        capacity = aligned_size;
        length = 0;
        owned = true;
    }

    // OPTIMIZATION: Wrap existing buffer (zero-copy)
    ALWAYS_INLINE
    void wrap(uint8_t* buf, size_t len) {
        if (owned && data) {
            free(data);
        }

        data = buf;
        capacity = len;
        length = len;
        owned = false;
    }
};

// ============================================================================
// OPTIMIZATION 6: Fast Handshake with Pre-computed Values
// ============================================================================

struct CACHE_ALIGNED TLSHandshakeCache {
    // Pre-computed DH parameters
    uint8_t dh_p[256];      // Prime modulus
    uint8_t dh_g[256];      // Generator
    uint8_t dh_private[32]; // Server private key
    uint8_t dh_public[256]; // Server public key

    // Pre-computed ECDH curve points
    uint8_t ecdh_private[32];
    uint8_t ecdh_public[65];

    // Server random (regenerated periodically)
    uint8_t server_random[32];
    uint64_t random_timestamp;

    TLSHandshakeCache() : random_timestamp(0) {
        memset(dh_p, 0, sizeof(dh_p));
        memset(dh_g, 0, sizeof(dh_g));
        memset(dh_private, 0, sizeof(dh_private));
        memset(dh_public, 0, sizeof(dh_public));
        memset(ecdh_private, 0, sizeof(ecdh_private));
        memset(ecdh_public, 0, sizeof(ecdh_public));
        memset(server_random, 0, sizeof(server_random));
    }

    // OPTIMIZATION: Get server random with lazy regeneration
    HOT_FUNCTION
    const uint8_t* get_server_random() {
        uint64_t now = get_timestamp();

        // Regenerate every 60 seconds
        if (UNLIKELY(now - random_timestamp > 60)) {
            generate_random(server_random, 32);
            random_timestamp = now;
        }

        return server_random;
    }

private:
    ALWAYS_INLINE uint64_t get_timestamp() const {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
    }

    void generate_random(uint8_t* buf, size_t len);
};

// ============================================================================
// OPTIMIZATION 7: Batch Certificate Verification
// ============================================================================

struct CertificateCache {
    std::unordered_map<std::string, bool> verified_certs;

    // OPTIMIZATION: Cache certificate verification results
    HOT_FUNCTION
    bool is_verified(const std::string& cert_hash) {
        auto it = verified_certs.find(cert_hash);
        return it != verified_certs.end() && it->second;
    }

    void mark_verified(const std::string& cert_hash, bool valid) {
        verified_certs[cert_hash] = valid;
    }
};

// ============================================================================
// OPTIMIZATION 8: Connection Pool for TLS Sessions
// ============================================================================

struct CACHE_ALIGNED TLSConnection {
    int socket_fd;
    TLSSession* session;
    TLSBuffer send_buffer;
    TLSBuffer recv_buffer;
    uint64_t last_activity;
    bool in_use;

    TLSConnection() : socket_fd(-1), session(nullptr), last_activity(0), in_use(false) {}
};

class TLSConnectionPool {
private:
    static constexpr size_t POOL_SIZE = 1000;
    CACHE_ALIGNED TLSConnection pool_[POOL_SIZE];

public:
    // OPTIMIZATION: Fast connection acquisition
    HOT_FUNCTION
    TLSConnection* acquire() {
        for (size_t i = 0; i < POOL_SIZE; i++) {
            if (!pool_[i].in_use) {
                pool_[i].in_use = true;
                return &pool_[i];
            }
        }
        return nullptr;
    }

    // OPTIMIZATION: Fast connection release
    HOT_FUNCTION
    void release(TLSConnection* conn) {
        if (conn) {
            conn->in_use = false;
        }
    }
};

// ============================================================================
// OPTIMIZATION 9: Kernel TLS (kTLS) Support
// ============================================================================

#ifdef __linux__

#include <linux/tls.h>

// Enable kernel TLS offload
HOT_FUNCTION
int enable_ktls(int socket_fd, const uint8_t* key, size_t key_len,
                const uint8_t* iv, size_t iv_len) {
    struct tls12_crypto_info_aes_gcm_128 crypto_info = {};

    crypto_info.info.version = TLS_1_2_VERSION;
    crypto_info.info.cipher_type = TLS_CIPHER_AES_GCM_128;

    memcpy(crypto_info.key, key, key_len);
    memcpy(crypto_info.iv, iv, iv_len);
    memcpy(crypto_info.rec_seq, "\x00\x00\x00\x00\x00\x00\x00\x00", 8);
    memcpy(crypto_info.salt, iv, 4);

    // Enable TX offload
    if (setsockopt(socket_fd, SOL_TLS, TLS_TX, &crypto_info, sizeof(crypto_info)) < 0) {
        return -1;
    }

    // Enable RX offload
    if (setsockopt(socket_fd, SOL_TLS, TLS_RX, &crypto_info, sizeof(crypto_info)) < 0) {
        return -1;
    }

    return 0;
}

#endif // __linux__

// ============================================================================
// OPTIMIZATION 10: Fast HKDF (HMAC-based Key Derivation)
// ============================================================================

// OPTIMIZATION: Unrolled HMAC-SHA256 for key derivation
HOT_FUNCTION
void hkdf_expand_fast(const uint8_t* prk, size_t prk_len,
                     const uint8_t* info, size_t info_len,
                     uint8_t* okm, size_t okm_len) {
    // Optimized HKDF implementation
    // TODO: Implement with SIMD acceleration
}

// ============================================================================
// Global TLS optimization state
// ============================================================================

extern TLSSessionCache g_tls_session_cache;
extern TLSHandshakeCache g_tls_handshake_cache;
extern CertificateCache g_cert_cache;
extern TLSConnectionPool g_tls_conn_pool;

} // namespace nova::tls
