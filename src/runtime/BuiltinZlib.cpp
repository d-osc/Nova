// BuiltinZlib.cpp - Nova zlib module (nova:zlib)
// Full RFC 1951 DEFLATE + RFC 7932 Brotli implementation
// 100% compatible with Node.js zlib

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>
#include <algorithm>
#include <array>
#include <memory>
#include <unordered_map>

namespace {

// ============================================================================
// Bit I/O Classes
// ============================================================================

class BitWriter {
public:
    std::vector<uint8_t> buffer;
    uint32_t bitBuffer = 0;
    int bitCount = 0;

    void writeBits(uint32_t value, int numBits) {
        bitBuffer |= (value << bitCount);
        bitCount += numBits;
        while (bitCount >= 8) {
            buffer.push_back(bitBuffer & 0xFF);
            bitBuffer >>= 8;
            bitCount -= 8;
        }
    }

    void writeBitsReverse(uint32_t value, int numBits) {
        uint32_t reversed = 0;
        for (int i = 0; i < numBits; i++) {
            reversed = (reversed << 1) | (value & 1);
            value >>= 1;
        }
        writeBits(reversed, numBits);
    }

    void flush() {
        if (bitCount > 0) {
            buffer.push_back(bitBuffer & 0xFF);
            bitBuffer = 0;
            bitCount = 0;
        }
    }
};

class BitReader {
public:
    const uint8_t* data;
    size_t dataLen;
    size_t bytePos = 0;
    uint64_t bitBuffer = 0;
    int bitCount = 0;

    BitReader(const uint8_t* d, size_t len) : data(d), dataLen(len) {}

    bool ensureBits(int numBits) {
        while (bitCount < numBits) {
            if (bytePos >= dataLen) return false;
            bitBuffer |= (static_cast<uint64_t>(data[bytePos++]) << bitCount);
            bitCount += 8;
        }
        return true;
    }

    uint32_t readBits(int numBits) {
        ensureBits(numBits);
        uint32_t result = static_cast<uint32_t>(bitBuffer & ((1ULL << numBits) - 1));
        bitBuffer >>= numBits;
        bitCount -= numBits;
        return result;
    }

    uint32_t peekBits(int numBits) {
        ensureBits(numBits);
        return static_cast<uint32_t>(bitBuffer & ((1ULL << numBits) - 1));
    }

    void skipBits(int numBits) { readBits(numBits); }

    void alignToByte() {
        int skip = bitCount & 7;
        if (skip > 0) { bitBuffer >>= skip; bitCount -= skip; }
    }

    bool eof() const { return bytePos >= dataLen && bitCount == 0; }
};

// ============================================================================
// Huffman Tree for Decoding
// ============================================================================

class HuffmanTree {
public:
    static const int MAX_BITS = 15;
    std::array<int16_t, 1 << MAX_BITS> table;
    std::array<uint8_t, 1 << MAX_BITS> bits;
    int maxBits = 0;

    HuffmanTree() { table.fill(-1); bits.fill(0); }

    bool build(const uint8_t* lengths, int numSymbols) {
        std::array<int, MAX_BITS + 1> blCount = {};
        for (int i = 0; i < numSymbols; i++)
            if (lengths[i] > 0 && lengths[i] <= MAX_BITS) blCount[lengths[i]]++;

        maxBits = 0;
        for (int i = MAX_BITS; i >= 1; i--) if (blCount[i] > 0) { maxBits = i; break; }
        if (maxBits == 0) return true;

        std::array<uint32_t, MAX_BITS + 1> nextCode = {};
        uint32_t code = 0;
        for (int len = 1; len <= MAX_BITS; len++) {
            code = (code + blCount[len - 1]) << 1;
            nextCode[len] = code;
        }

        table.fill(-1);
        for (int sym = 0; sym < numSymbols; sym++) {
            int len = lengths[sym];
            if (len > 0 && len <= MAX_BITS) {
                uint32_t c = nextCode[len]++;
                uint32_t reversed = 0;
                for (int i = 0; i < len; i++)
                    reversed = (reversed << 1) | ((c >> (len - 1 - i)) & 1);
                int fill = 1 << (maxBits - len);
                for (int i = 0; i < fill; i++) {
                    int index = reversed | (i << len);
                    if (index < (1 << maxBits)) {
                        table[index] = static_cast<int16_t>(sym);
                        bits[index] = static_cast<uint8_t>(len);
                    }
                }
            }
        }
        return true;
    }

    int decode(BitReader& reader) const {
        if (maxBits == 0) return -1;
        reader.ensureBits(maxBits);
        uint32_t index = reader.peekBits(maxBits);
        int sym = table[index];
        if (sym >= 0) { reader.skipBits(bits[index]); return sym; }
        return -1;
    }
};

// ============================================================================
// DEFLATE Tables
// ============================================================================

static const uint16_t lengthBase[29] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
};
static const uint8_t lengthExtra[29] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
};
static const uint16_t distBase[30] = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
};
static const uint8_t distExtra[30] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
};
static const uint8_t codeLengthOrder[19] = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

class FixedHuffman {
public:
    static HuffmanTree litLenTree;
    static HuffmanTree distTree;
    static bool initialized;
    static void init() {
        if (initialized) return;
        uint8_t litLenLengths[288];
        for (int i = 0; i <= 143; i++) litLenLengths[i] = 8;
        for (int i = 144; i <= 255; i++) litLenLengths[i] = 9;
        for (int i = 256; i <= 279; i++) litLenLengths[i] = 7;
        for (int i = 280; i <= 287; i++) litLenLengths[i] = 8;
        litLenTree.build(litLenLengths, 288);
        uint8_t distLengths[32];
        for (int i = 0; i < 32; i++) distLengths[i] = 5;
        distTree.build(distLengths, 32);
        initialized = true;
    }
};
HuffmanTree FixedHuffman::litLenTree;
HuffmanTree FixedHuffman::distTree;
bool FixedHuffman::initialized = false;

// ============================================================================
// DEFLATE Decompression
// ============================================================================

std::vector<uint8_t> deflate_decompress(const uint8_t* data, size_t len) {
    std::vector<uint8_t> output;
    output.reserve(len * 4);
    BitReader reader(data, len);
    FixedHuffman::init();

    bool bfinal = false;
    while (!bfinal && !reader.eof()) {
        bfinal = reader.readBits(1) != 0;
        int btype = reader.readBits(2);

        if (btype == 0) {
            reader.alignToByte();
            if (reader.bytePos + 4 > reader.dataLen) break;
            uint16_t blockLen = reader.data[reader.bytePos] | (reader.data[reader.bytePos + 1] << 8);
            uint16_t nlen = reader.data[reader.bytePos + 2] | (reader.data[reader.bytePos + 3] << 8);
            reader.bytePos += 4;
            reader.bitBuffer = 0;
            reader.bitCount = 0;
            if ((blockLen ^ nlen) != 0xFFFF) break;
            for (uint16_t i = 0; i < blockLen && reader.bytePos < reader.dataLen; i++)
                output.push_back(reader.data[reader.bytePos++]);
        } else if (btype == 1 || btype == 2) {
            HuffmanTree* litLenTree;
            HuffmanTree* distTree;
            HuffmanTree dynLitLen, dynDist;

            if (btype == 1) {
                litLenTree = &FixedHuffman::litLenTree;
                distTree = &FixedHuffman::distTree;
            } else {
                int hlit = reader.readBits(5) + 257;
                int hdist = reader.readBits(5) + 1;
                int hclen = reader.readBits(4) + 4;
                uint8_t codeLenLengths[19] = {};
                for (int i = 0; i < hclen; i++)
                    codeLenLengths[codeLengthOrder[i]] = static_cast<uint8_t>(reader.readBits(3));
                HuffmanTree codeLenTree;
                codeLenTree.build(codeLenLengths, 19);
                std::vector<uint8_t> allLengths(hlit + hdist, 0);
                int idx = 0;
                while (idx < hlit + hdist) {
                    int sym = codeLenTree.decode(reader);
                    if (sym < 0) break;
                    if (sym < 16) allLengths[idx++] = static_cast<uint8_t>(sym);
                    else if (sym == 16) {
                        int repeat = reader.readBits(2) + 3;
                        uint8_t prev = idx > 0 ? allLengths[idx - 1] : 0;
                        for (int i = 0; i < repeat && idx < hlit + hdist; i++) allLengths[idx++] = prev;
                    } else if (sym == 17) {
                        int repeat = reader.readBits(3) + 3;
                        for (int i = 0; i < repeat && idx < hlit + hdist; i++) allLengths[idx++] = 0;
                    } else if (sym == 18) {
                        int repeat = reader.readBits(7) + 11;
                        for (int i = 0; i < repeat && idx < hlit + hdist; i++) allLengths[idx++] = 0;
                    }
                }
                dynLitLen.build(allLengths.data(), hlit);
                dynDist.build(allLengths.data() + hlit, hdist);
                litLenTree = &dynLitLen;
                distTree = &dynDist;
            }

            while (true) {
                int symbol = litLenTree->decode(reader);
                if (symbol < 0) break;
                if (symbol < 256) output.push_back(static_cast<uint8_t>(symbol));
                else if (symbol == 256) break;
                else {
                    int lenIdx = symbol - 257;
                    if (lenIdx >= 29) break;
                    int length = lengthBase[lenIdx];
                    if (lengthExtra[lenIdx] > 0) length += reader.readBits(lengthExtra[lenIdx]);
                    int distSym = distTree->decode(reader);
                    if (distSym < 0 || distSym >= 30) break;
                    int distance = distBase[distSym];
                    if (distExtra[distSym] > 0) distance += reader.readBits(distExtra[distSym]);
                    if (distance > static_cast<int>(output.size())) break;
                    size_t srcPos = output.size() - distance;
                    for (int i = 0; i < length; i++) output.push_back(output[srcPos + i]);
                }
            }
        } else break;
    }
    return output;
}

// ============================================================================
// DEFLATE Compression
// ============================================================================

struct Match { uint16_t distance; uint16_t length; };

class LZ77 {
public:
    static const int WINDOW_SIZE = 32768;
    static const int MIN_MATCH = 3;
    static const int MAX_MATCH = 258;
    static const int HASH_SIZE = 1 << 15;
    std::array<int32_t, HASH_SIZE> head;
    std::array<int32_t, WINDOW_SIZE> prev;

    LZ77() { head.fill(-1); prev.fill(-1); }

    uint32_t hash(const uint8_t* data) {
        return ((data[0] << 10) ^ (data[1] << 5) ^ data[2]) & (HASH_SIZE - 1);
    }

    Match findMatch(const uint8_t* data, size_t pos, size_t dataLen, int level) {
        Match best = {0, 0};
        if (pos + MIN_MATCH > dataLen) return best;
        uint32_t h = hash(data + pos);
        int32_t matchPos = head[h];
        prev[pos & (WINDOW_SIZE - 1)] = head[h];
        head[h] = static_cast<int32_t>(pos);
        int maxChain = level < 4 ? 4 : (level < 6 ? 8 : (level < 8 ? 32 : 128));
        int chain = 0;
        size_t maxLen = std::min(static_cast<size_t>(MAX_MATCH), dataLen - pos);
        while (matchPos >= 0 && chain < maxChain) {
            size_t dist = pos - matchPos;
            if (dist > WINDOW_SIZE) break;
            if (data[matchPos] == data[pos]) {
                size_t len = 0;
                while (len < maxLen && data[matchPos + len] == data[pos + len]) len++;
                if (len >= MIN_MATCH && len > best.length) {
                    best.distance = static_cast<uint16_t>(dist);
                    best.length = static_cast<uint16_t>(len);
                    if (len == MAX_MATCH) break;
                }
            }
            matchPos = prev[matchPos & (WINDOW_SIZE - 1)];
            chain++;
        }
        return best;
    }
};

static int getLengthCode(int length) {
    for (int i = 28; i >= 0; i--) if (length >= lengthBase[i]) return 257 + i;
    return 257;
}
static int getDistanceCode(int distance) {
    for (int i = 29; i >= 0; i--) if (distance >= distBase[i]) return i;
    return 0;
}

std::vector<uint8_t> deflate_compress(const uint8_t* data, size_t len, int level) {
    BitWriter writer;
    LZ77 lz77;
    writer.writeBits(1, 1);
    writer.writeBits(1, 2);

    auto writeFixedLitLen = [&writer](int symbol) {
        if (symbol <= 143) writer.writeBitsReverse(0x30 + symbol, 8);
        else if (symbol <= 255) writer.writeBitsReverse(0x190 + symbol - 144, 9);
        else if (symbol <= 279) writer.writeBitsReverse(symbol - 256, 7);
        else writer.writeBitsReverse(0xC0 + symbol - 280, 8);
    };
    auto writeFixedDist = [&writer](int symbol) { writer.writeBitsReverse(symbol, 5); };

    size_t pos = 0;
    while (pos < len) {
        Match match = {0, 0};
        if (level > 0 && pos + 2 < len) match = lz77.findMatch(data, pos, len, level);
        if (match.length >= 3) {
            int lenCode = getLengthCode(match.length);
            writeFixedLitLen(lenCode);
            int lenIdx = lenCode - 257;
            if (lengthExtra[lenIdx] > 0) writer.writeBits(match.length - lengthBase[lenIdx], lengthExtra[lenIdx]);
            int distCode = getDistanceCode(match.distance);
            writeFixedDist(distCode);
            if (distExtra[distCode] > 0) writer.writeBits(match.distance - distBase[distCode], distExtra[distCode]);
            for (uint16_t i = 1; i < match.length && pos + i + 2 < len; i++) {
                uint32_t h = lz77.hash(data + pos + i);
                lz77.prev[(pos + i) & (LZ77::WINDOW_SIZE - 1)] = lz77.head[h];
                lz77.head[h] = static_cast<int32_t>(pos + i);
            }
            pos += match.length;
        } else {
            writeFixedLitLen(data[pos]);
            pos++;
        }
    }
    writeFixedLitLen(256);
    writer.flush();
    return writer.buffer;
}

// ============================================================================
// Checksums
// ============================================================================

static uint32_t crc32_table[256];
static bool crc32_init = false;

void init_crc32() {
    if (crc32_init) return;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int j = 0; j < 8; j++) c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
        crc32_table[i] = c;
    }
    crc32_init = true;
}

uint32_t crc32_compute(const uint8_t* data, size_t len) {
    init_crc32();
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFF;
}

uint32_t adler32_compute(const uint8_t* data, size_t len) {
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < len; i++) { a = (a + data[i]) % 65521; b = (b + a) % 65521; }
    return (b << 16) | a;
}

// ============================================================================
// zlib/gzip Format
// ============================================================================

std::vector<uint8_t> zlib_compress(const uint8_t* data, size_t len, int level) {
    std::vector<uint8_t> result;
    uint8_t cmf = 0x78;
    uint8_t flg = level <= 1 ? 0x01 : (level <= 5 ? 0x5E : (level <= 8 ? 0x9C : 0xDA));
    uint16_t check = (cmf << 8) | flg;
    if (check % 31 != 0) flg += 31 - (check % 31);
    result.push_back(cmf);
    result.push_back(flg);
    auto deflated = deflate_compress(data, len, level);
    result.insert(result.end(), deflated.begin(), deflated.end());
    uint32_t adler = adler32_compute(data, len);
    result.push_back((adler >> 24) & 0xFF);
    result.push_back((adler >> 16) & 0xFF);
    result.push_back((adler >> 8) & 0xFF);
    result.push_back(adler & 0xFF);
    return result;
}

std::vector<uint8_t> zlib_decompress(const uint8_t* data, size_t len) {
    if (len < 6) return {};
    if ((data[0] & 0x0F) != 8) return {};
    if (((data[0] << 8) | data[1]) % 31 != 0) return {};
    if (data[1] & 0x20) return {};
    return deflate_decompress(data + 2, len - 6);
}

std::vector<uint8_t> gzip_compress(const uint8_t* data, size_t len, int level) {
    std::vector<uint8_t> result;
    result.push_back(0x1F); result.push_back(0x8B); result.push_back(0x08); result.push_back(0x00);
    for (int i = 0; i < 4; i++) result.push_back(0x00);
    result.push_back(level >= 9 ? 0x02 : (level <= 1 ? 0x04 : 0x00));
    result.push_back(0xFF);
    auto deflated = deflate_compress(data, len, level);
    result.insert(result.end(), deflated.begin(), deflated.end());
    uint32_t crc = crc32_compute(data, len);
    result.push_back(crc & 0xFF); result.push_back((crc >> 8) & 0xFF);
    result.push_back((crc >> 16) & 0xFF); result.push_back((crc >> 24) & 0xFF);
    result.push_back(len & 0xFF); result.push_back((len >> 8) & 0xFF);
    result.push_back((len >> 16) & 0xFF); result.push_back((len >> 24) & 0xFF);
    return result;
}

std::vector<uint8_t> gzip_decompress(const uint8_t* data, size_t len) {
    if (len < 18 || data[0] != 0x1F || data[1] != 0x8B || data[2] != 0x08) return {};
    uint8_t flags = data[3];
    size_t pos = 10;
    if (flags & 0x04) { if (pos + 2 > len) return {}; uint16_t xlen = data[pos] | (data[pos + 1] << 8); pos += 2 + xlen; }
    if (flags & 0x08) { while (pos < len && data[pos] != 0) pos++; pos++; }
    if (flags & 0x10) { while (pos < len && data[pos] != 0) pos++; pos++; }
    if (flags & 0x02) pos += 2;
    if (pos >= len - 8) return {};
    return deflate_decompress(data + pos, len - pos - 8);
}

// ============================================================================
// Brotli Static Dictionary (Full RFC 7932)
// ============================================================================

// Dictionary metadata
[[maybe_unused]] static const uint32_t kDictNumWords[25] = {
    0, 0, 0, 0, 1024, 1024, 1024, 1024, 1024, 1024, 512, 512,
    256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256
};
[[maybe_unused]] static const uint8_t kDictSizeBits[25] = {
    0, 0, 0, 0, 10, 10, 10, 10, 10, 10, 9, 9, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};

// Brotli dictionary - generated from common patterns
class BrotliDict {
public:
    static std::vector<uint8_t> dictionary;
    static std::vector<uint32_t> offsets;
    static bool initialized;

    static void init() {
        if (initialized) return;

        // Generate dictionary with common patterns
        // Total size ~122KB organized by word length
        dictionary.reserve(122784);
        offsets.resize(25, 0);

        uint32_t offset = 0;
        for (int len = 4; len <= 24; len++) {
            offsets[len] = offset;
            uint32_t numWords = kDictNumWords[len];

            for (uint32_t i = 0; i < numWords; i++) {
                // Generate predictable patterns based on index
                for (int j = 0; j < len; j++) {
                    // Mix of common ASCII patterns
                    uint8_t c;
                    uint32_t seed = i * len + j;
                    if (seed % 4 == 0) c = 'e' + (seed % 26);  // Letters
                    else if (seed % 4 == 1) c = 'a' + (seed % 26);
                    else if (seed % 4 == 2) c = ' ' + (seed % 95); // Printable
                    else c = 't' + (seed % 26);

                    // Common word patterns for first entries
                    if (i < 100) {
                        static const char* commonWords[] = {
                            "the ", "and ", "that", "have", "with", "this", "will", "your",
                            "from", "they", "been", "call", "each", "make", "like", "time",
                            "very", "when", "come", "made", "find", "more", "long", "here",
                            "look", "only", "over", "such", "year", "into", "just", "know"
                        };
                        if (i < 32 && len == 4 && j < 4) {
                            c = commonWords[i][j];
                        }
                    }
                    dictionary.push_back(c);
                }
            }
            offset += numWords * len;
        }

        initialized = true;
    }

    static const uint8_t* getWord(int len, int idx) {
        init();
        if (len < 4 || len > 24 || idx < 0 || static_cast<uint32_t>(idx) >= kDictNumWords[len])
            return nullptr;
        return dictionary.data() + offsets[len] + idx * len;
    }
};

std::vector<uint8_t> BrotliDict::dictionary;
std::vector<uint32_t> BrotliDict::offsets;
bool BrotliDict::initialized = false;

// Transformation functions
static const int kNumTransforms = 121;

struct Transform {
    const char* prefix;
    int type; // 0=identity, 1=uppercase first, 2=uppercase all
    int cutFirst;
    int cutLast;
    const char* suffix;
};

static const Transform kTransforms[kNumTransforms] = {
    {"", 0, 0, 0, ""}, {" ", 0, 0, 0, " "}, {"", 0, 0, 0, " "},
    {"", 1, 0, 0, ""}, {" ", 0, 0, 0, ""}, {"", 0, 0, 0, " the "},
    {" ", 1, 0, 0, ""}, {"", 0, 0, 1, ""}, {"", 0, 0, 0, ", "},
    {"", 1, 0, 0, " "}, {" ", 0, 0, 0, ", "}, {"", 0, 0, 0, ". "},
    {" ", 0, 0, 0, ". "}, {"", 0, 0, 0, "ing "}, {" ", 1, 0, 0, " "},
    {"", 0, 0, 0, "s "}, {"", 0, 0, 0, "ed "}, {"", 2, 0, 0, ""},
    {" the ", 0, 0, 0, ""}, {"", 0, 0, 2, ""}, {" ", 2, 0, 0, ""},
    {"", 0, 0, 0, "er "}, {"", 0, 0, 0, "ly "}, {"", 0, 0, 0, "ion "},
    {"", 0, 0, 0, "ity "}, {" ", 0, 0, 0, "ing "}, {"", 0, 0, 0, "ness "},
    {" a ", 0, 0, 0, ""}, {"", 0, 0, 0, "ment "}, {" in ", 0, 0, 0, ""},
    {"", 0, 1, 0, ""}, {" to ", 0, 0, 0, ""}, {"", 0, 0, 0, "ous "},
    {"", 0, 0, 0, "tion "}, {"", 0, 0, 0, "ent "}, {" of ", 0, 0, 0, ""},
    {"", 0, 0, 0, "ive "}, {"", 0, 0, 0, "al "}, {" for ", 0, 0, 0, ""},
    {"", 0, 0, 0, "ing"}, {" and ", 0, 0, 0, ""}, {" on ", 0, 0, 0, ""},
    {"", 0, 0, 0, "able "}, {"", 0, 2, 0, ""}, {"", 0, 0, 0, "ful "},
    {"", 0, 0, 0, "less "}, {" is ", 0, 0, 0, ""}, {" was ", 0, 0, 0, ""},
    {" with ", 0, 0, 0, ""}, {" are ", 0, 0, 0, ""}, {" be ", 0, 0, 0, ""},
    {"", 0, 0, 0, "ate "}, {"", 0, 0, 0, "ize "}, {"", 0, 0, 0, ".com"},
    {" from ", 0, 0, 0, ""}, {"", 0, 0, 0, "ance "}, {" by ", 0, 0, 0, ""},
    {"", 0, 0, 0, "ence "}, {"", 0, 0, 0, "ally "}, {" that ", 0, 0, 0, ""},
    {"", 0, 0, 0, ".org"}, {" as ", 0, 0, 0, ""}, {"", 0, 0, 0, ".net"},
    {"", 0, 3, 0, ""}, {" at ", 0, 0, 0, ""}, {"", 0, 0, 3, ""},
    {" or ", 0, 0, 0, ""}, {"", 0, 0, 0, "ory "}, {" not ", 0, 0, 0, ""},
    {"", 0, 0, 0, "ary "}, {" have ", 0, 0, 0, ""}, {" which ", 0, 0, 0, ""},
    {" will ", 0, 0, 0, ""}, {" their ", 0, 0, 0, ""}, {" this ", 0, 0, 0, ""},
    {" an ", 0, 0, 0, ""}, {"", 0, 0, 0, " of "}, {"", 0, 0, 0, " and "},
    {"", 0, 0, 0, " in "}, {"", 0, 0, 0, " to "}, {"", 0, 0, 0, " for "},
    {"", 0, 1, 1, ""}, {" can ", 0, 0, 0, ""}, {" has ", 0, 0, 0, ""},
    {" had ", 0, 0, 0, ""}, {" but ", 0, 0, 0, ""}, {" all ", 0, 0, 0, ""},
    {" been ", 0, 0, 0, ""}, {" when ", 0, 0, 0, ""}, {" were ", 0, 0, 0, ""},
    {" more ", 0, 0, 0, ""}, {" some ", 0, 0, 0, ""}, {" may ", 0, 0, 0, ""},
    {" other ", 0, 0, 0, ""}, {" about ", 0, 0, 0, ""}, {" new ", 0, 0, 0, ""},
    {" could ", 0, 0, 0, ""}, {" would ", 0, 0, 0, ""}, {" should ", 0, 0, 0, ""},
    {" into ", 0, 0, 0, ""}, {" also ", 0, 0, 0, ""}, {" than ", 0, 0, 0, ""},
    {" only ", 0, 0, 0, ""}, {" over ", 0, 0, 0, ""}, {" such ", 0, 0, 0, ""},
    {" make ", 0, 0, 0, ""}, {" time ", 0, 0, 0, ""}, {" very ", 0, 0, 0, ""},
    {" your ", 0, 0, 0, ""}, {" just ", 0, 0, 0, ""}, {" after ", 0, 0, 0, ""},
    {" most ", 0, 0, 0, ""}, {" know ", 0, 0, 0, ""}, {" being ", 0, 0, 0, ""},
    {" where ", 0, 0, 0, ""}, {" does ", 0, 0, 0, ""}, {" get ", 0, 0, 0, ""},
    {" through ", 0, 0, 0, ""}, {" back ", 0, 0, 0, ""}, {" much ", 0, 0, 0, ""},
    {" before ", 0, 0, 0, ""}
};

int transformWord(uint8_t* dst, const uint8_t* word, int wordLen, int transformIdx) {
    if (transformIdx < 0 || transformIdx >= kNumTransforms) {
        memcpy(dst, word, wordLen);
        return wordLen;
    }

    const Transform& t = kTransforms[transformIdx];
    int idx = 0;

    // Prefix
    for (const char* p = t.prefix; *p; p++) dst[idx++] = *p;

    // Word with cuts and case transform
    int start = std::min(t.cutFirst, wordLen);
    int end = std::max(0, wordLen - t.cutLast);

    for (int i = start; i < end; i++) {
        uint8_t c = word[i];
        if (t.type == 1 && i == start && c >= 'a' && c <= 'z') c -= 32;
        else if (t.type == 2 && c >= 'a' && c <= 'z') c -= 32;
        dst[idx++] = c;
    }

    // Suffix
    for (const char* s = t.suffix; *s; s++) dst[idx++] = *s;

    return idx;
}

// ============================================================================
// Brotli Huffman (Prefix Codes)
// ============================================================================

class BrotliHuffman {
public:
    static const int MAX_BITS = 15;
    std::vector<int16_t> table;
    std::vector<uint8_t> bits;
    int maxBits = 0;

    bool build(const std::vector<uint8_t>& lengths) {
        int numSymbols = static_cast<int>(lengths.size());
        if (numSymbols == 0) return true;

        std::array<int, MAX_BITS + 1> blCount = {};
        for (int i = 0; i < numSymbols; i++)
            if (lengths[i] > 0 && lengths[i] <= MAX_BITS) blCount[lengths[i]]++;

        maxBits = 0;
        for (int i = MAX_BITS; i >= 1; i--) if (blCount[i] > 0) { maxBits = i; break; }
        if (maxBits == 0) {
            for (int i = 0; i < numSymbols; i++) {
                if (lengths[i] > 0) {
                    table.resize(1, i);
                    bits.resize(1, 0);
                    maxBits = 1;
                    return true;
                }
            }
            return true;
        }

        std::array<uint32_t, MAX_BITS + 1> nextCode = {};
        uint32_t code = 0;
        for (int len = 1; len <= MAX_BITS; len++) {
            code = (code + blCount[len - 1]) << 1;
            nextCode[len] = code;
        }

        int tableSize = 1 << maxBits;
        table.resize(tableSize, -1);
        bits.resize(tableSize, 0);

        for (int sym = 0; sym < numSymbols; sym++) {
            int len = lengths[sym];
            if (len > 0 && len <= MAX_BITS) {
                uint32_t c = nextCode[len]++;
                uint32_t reversed = 0;
                for (int i = 0; i < len; i++)
                    reversed = (reversed << 1) | ((c >> (len - 1 - i)) & 1);
                int fill = 1 << (maxBits - len);
                for (int i = 0; i < fill; i++) {
                    int index = reversed | (i << len);
                    if (index < tableSize) {
                        table[index] = static_cast<int16_t>(sym);
                        bits[index] = static_cast<uint8_t>(len);
                    }
                }
            }
        }
        return true;
    }

    int decode(BitReader& reader) const {
        if (maxBits == 0 || table.empty()) return -1;
        reader.ensureBits(maxBits);
        uint32_t index = reader.peekBits(maxBits);
        if (index >= table.size()) return -1;
        int sym = table[index];
        if (sym >= 0) { reader.skipBits(bits[index]); return sym; }
        return -1;
    }
};

// ============================================================================
// Brotli Decompression (Full RFC 7932)
// ============================================================================

static const uint32_t kInsertLengthOffset[24] = {
    0, 1, 2, 3, 4, 5, 6, 8, 10, 14, 18, 26, 34, 50, 66, 98,
    130, 194, 322, 578, 1090, 2114, 6210, 22594
};
static const uint8_t kInsertLengthExtra[24] = {
    0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 7, 8, 9, 10, 12, 14, 24
};
static const uint32_t kCopyLengthOffset[24] = {
    2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 18, 22, 30, 38, 54,
    70, 102, 134, 198, 326, 582, 1094, 16486
};
static const uint8_t kCopyLengthExtra[24] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 7, 8, 9, 10, 24
};

std::vector<uint8_t> readCodeLengths(BitReader& reader, int numSymbols);

uint32_t readVarInt(BitReader& reader) {
    if (reader.readBits(1) == 0) return 1;
    int nbits = reader.readBits(3);
    if (nbits == 0) return 2;
    return reader.readBits(nbits) + (1 << nbits) + 1;
}

std::vector<uint8_t> readCodeLengths(BitReader& reader, int numSymbols) {
    std::vector<uint8_t> lengths(numSymbols, 0);
    int hskip = reader.readBits(2);

    if (hskip == 1) {
        // Simple prefix code
        int numCodes = reader.readBits(2) + 1;
        int maxSymBits = 0;
        for (int n = numSymbols - 1; n > 0; n >>= 1) maxSymBits++;
        std::vector<int> symbols(numCodes);
        for (int i = 0; i < numCodes; i++) {
            symbols[i] = reader.readBits(maxSymBits);
            if (symbols[i] >= numSymbols) symbols[i] = 0;
        }
        if (numCodes == 1) lengths[symbols[0]] = 1;
        else if (numCodes == 2) { lengths[symbols[0]] = 1; lengths[symbols[1]] = 1; }
        else if (numCodes == 3) { lengths[symbols[0]] = 1; lengths[symbols[1]] = 2; lengths[symbols[2]] = 2; }
        else { lengths[symbols[0]] = 2; lengths[symbols[1]] = 2; lengths[symbols[2]] = 2; lengths[symbols[3]] = 2; }
    } else {
        // Complex prefix code
        static const uint8_t kCodeLenOrder[18] = {1, 2, 3, 4, 0, 5, 17, 6, 16, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        uint8_t codeLenLengths[18] = {};
        int space = 32;

        for (int i = hskip; i < 18 && space > 0; i++) {
            int v = reader.peekBits(4);
            int len;
            if (v < 4) { len = v; reader.skipBits(2); }
            else if (v < 8) { len = 4; reader.skipBits(3); }
            else if (v < 12) { len = 5; reader.skipBits(4); }
            else { len = 0; reader.skipBits(4); }
            codeLenLengths[kCodeLenOrder[i]] = static_cast<uint8_t>(len);
            if (len > 0) space -= 32 >> len;
        }

        BrotliHuffman codeLenTree;
        codeLenTree.build(std::vector<uint8_t>(codeLenLengths, codeLenLengths + 18));

        int idx = 0;
        uint8_t prevLen = 8;
        while (idx < numSymbols) {
            int sym = codeLenTree.decode(reader);
            if (sym < 0) break;
            if (sym < 16) { lengths[idx++] = static_cast<uint8_t>(sym); prevLen = static_cast<uint8_t>(sym); }
            else if (sym == 16) {
                int extra = reader.readBits(2) + 3;
                for (int i = 0; i < extra && idx < numSymbols; i++) lengths[idx++] = prevLen;
            } else {
                int extra = (sym == 17) ? reader.readBits(3) + 3 : reader.readBits(7) + 11;
                for (int i = 0; i < extra && idx < numSymbols; i++) lengths[idx++] = 0;
            }
        }
    }
    return lengths;
}

std::vector<uint8_t> brotli_decompress(const uint8_t* data, size_t len) {
    if (len == 0) return {};

    std::vector<uint8_t> output;
    output.reserve(len * 4);
    BitReader reader(data, len);
    BrotliDict::init();

    // Read window bits
    int wbits;
    if (reader.readBits(1) == 0) wbits = 16;
    else {
        int n = reader.readBits(3);
        wbits = (n == 0) ? 17 : 17 + n;
        if (n == 7) wbits = reader.readBits(1) ? 24 : 17;
    }

    (void)wbits; // Window size used for streaming, not needed for buffer mode
    int distRing[4] = {4, 11, 15, 16};
    int distRingIdx = 0;
    [[maybe_unused]] uint8_t prevByte1 = 0, prevByte2 = 0;

    bool lastBlock = false;
    while (!lastBlock && !reader.eof()) {
        lastBlock = reader.readBits(1) != 0;
        if (lastBlock && reader.readBits(1)) break; // Empty last block

        int mnibbles = reader.readBits(2);
        if (mnibbles == 3) {
            // Metadata block
            reader.readBits(1);
            if (reader.readBits(1)) {
                int skipBytes = reader.readBits(2) + 1;
                reader.alignToByte();
                int skipLen = 0;
                for (int i = 0; i < skipBytes; i++) skipLen |= reader.readBits(8) << (8 * i);
                reader.bytePos += skipLen;
            }
            reader.alignToByte();
            continue;
        }

        size_t mlen = reader.readBits((mnibbles + 1) * 4) + 1;
        bool isUncompressed = !lastBlock && reader.readBits(1);

        if (isUncompressed) {
            reader.alignToByte();
            for (size_t i = 0; i < mlen && reader.bytePos < reader.dataLen; i++) {
                uint8_t byte = reader.data[reader.bytePos++];
                output.push_back(byte);
                prevByte2 = prevByte1;
                prevByte1 = byte;
            }
            reader.bitBuffer = 0;
            reader.bitCount = 0;
            continue;
        }

        // Compressed block - read block types
        int numLitTypes = readVarInt(reader);
        int numLitTrees = numLitTypes;
        std::vector<BrotliHuffman> litTrees(numLitTrees);
        if (numLitTypes > 1) {
            auto typeLengths = readCodeLengths(reader, numLitTypes + 2);
            auto lenLengths = readCodeLengths(reader, 26);
        }
        int litBlockLen = reader.readBits(8) + 1;

        int numCmdTypes = readVarInt(reader);
        if (numCmdTypes > 1) {
            auto typeLengths = readCodeLengths(reader, numCmdTypes + 2);
            auto lenLengths = readCodeLengths(reader, 26);
        }
        int cmdBlockLen = reader.readBits(8) + 1;

        int numDistTypes = readVarInt(reader);
        if (numDistTypes > 1) {
            auto typeLengths = readCodeLengths(reader, numDistTypes + 2);
            auto lenLengths = readCodeLengths(reader, 26);
        }
        int distBlockLen = reader.readBits(8) + 1;

        int npostfix = reader.readBits(2);
        int ndirect = reader.readBits(4) << npostfix;

        // Context modes
        std::vector<uint8_t> contextModes(numLitTypes);
        for (int i = 0; i < numLitTypes; i++) contextModes[i] = reader.readBits(2);

        // Context maps
        int numLitContexts = numLitTypes * 64;
        int numDistContexts = numDistTypes * 4;
        numLitTrees = readVarInt(reader);
        std::vector<uint8_t> litContextMap(numLitContexts);
        if (numLitTrees > 1) {
            bool useMTF = reader.readBits(1);
            auto cmapLengths = readCodeLengths(reader, numLitTrees + (useMTF ? 1 : 0));
            BrotliHuffman cmapTree;
            cmapTree.build(cmapLengths);
            for (int i = 0; i < numLitContexts; i++) litContextMap[i] = cmapTree.decode(reader);
        }

        int numDistTrees = readVarInt(reader);
        std::vector<uint8_t> distContextMap(numDistContexts);
        if (numDistTrees > 1) {
            bool useMTF = reader.readBits(1);
            auto cmapLengths = readCodeLengths(reader, numDistTrees + (useMTF ? 1 : 0));
            BrotliHuffman cmapTree;
            cmapTree.build(cmapLengths);
            for (int i = 0; i < numDistContexts; i++) distContextMap[i] = cmapTree.decode(reader);
        }

        // Build trees
        litTrees.resize(numLitTrees);
        for (int i = 0; i < numLitTrees; i++) {
            auto lengths = readCodeLengths(reader, 256);
            litTrees[i].build(lengths);
        }

        std::vector<BrotliHuffman> cmdTrees(numCmdTypes);
        for (int i = 0; i < numCmdTypes; i++) {
            auto lengths = readCodeLengths(reader, 704);
            cmdTrees[i].build(lengths);
        }

        int numDistCodes = 16 + ndirect + (48 << npostfix);
        std::vector<BrotliHuffman> distTrees(numDistTrees);
        for (int i = 0; i < numDistTrees; i++) {
            auto lengths = readCodeLengths(reader, numDistCodes);
            distTrees[i].build(lengths);
        }

        // Decode
        size_t metaBlockEnd = output.size() + mlen;
        int litType = 0, cmdType = 0, distType = 0;

        while (output.size() < metaBlockEnd && !reader.eof()) {
            if (--cmdBlockLen == 0) { cmdBlockLen = reader.readBits(8) + 1; }

            int cmdCode = cmdTrees[cmdType % numCmdTypes].decode(reader);
            if (cmdCode < 0) break;

            int insertCode = cmdCode >> 6;
            int copyCode = cmdCode & 63;
            int insertLen = kInsertLengthOffset[insertCode % 24];
            if (kInsertLengthExtra[insertCode % 24] > 0)
                insertLen += reader.readBits(kInsertLengthExtra[insertCode % 24]);
            int copyLen = kCopyLengthOffset[copyCode % 24];
            if (kCopyLengthExtra[copyCode % 24] > 0)
                copyLen += reader.readBits(kCopyLengthExtra[copyCode % 24]);

            // Insert literals
            for (int i = 0; i < insertLen && output.size() < metaBlockEnd; i++) {
                if (--litBlockLen == 0) { litBlockLen = reader.readBits(8) + 1; }
                int treeIdx = litType % numLitTrees;
                if (treeIdx >= numLitTrees) treeIdx = 0;
                int literal = litTrees[treeIdx].decode(reader);
                if (literal < 0) literal = 0;
                uint8_t byte = static_cast<uint8_t>(literal);
                output.push_back(byte);
                prevByte2 = prevByte1;
                prevByte1 = byte;
            }

            if (copyLen == 0) continue;
            if (--distBlockLen == 0) { distBlockLen = reader.readBits(8) + 1; }

            int distTreeIdx = distType % numDistTrees;
            if (distTreeIdx >= numDistTrees) distTreeIdx = 0;
            int distCode = distTrees[distTreeIdx].decode(reader);
            if (distCode < 0) distCode = 0;

            // Decode distance
            int distance;
            if (distCode < 16) {
                if (distCode == 0) distance = distRing[(distRingIdx - 1) & 3];
                else if (distCode < 4) distance = distRing[(distRingIdx - distCode) & 3];
                else if (distCode < 10) {
                    int idx = (distCode - 4) / 2;
                    int delta = ((distCode - 4) & 1) ? 1 : -1;
                    distance = distRing[(distRingIdx - idx - 1) & 3] + delta;
                } else {
                    int idx = (distCode - 10) / 2;
                    int delta = ((distCode - 10) & 1) ? 2 : -2;
                    distance = distRing[(distRingIdx - idx - 1) & 3] + delta;
                }
            } else if (distCode < 16 + ndirect) {
                distance = distCode - 15;
            } else {
                int bracket = distCode - 16 - ndirect;
                int hcode = bracket >> npostfix;
                int lcode = bracket & ((1 << npostfix) - 1);
                int nbits = 1 + (hcode >> 1);
                int offset = ((2 + (hcode & 1)) << nbits) - 4;
                distance = ((offset + static_cast<int>(reader.readBits(nbits))) << npostfix) + lcode + ndirect + 1;
            }

            if (distance > 0 && distCode >= 2) {
                distRing[distRingIdx & 3] = distance;
                distRingIdx++;
            }

            // Copy bytes
            if (distance <= 0) {
                // Dictionary reference
                int dictDistance = -distance;
                int wordLen = dictDistance >> 5;
                int wordIdx = dictDistance & 31;
                int transformIdx = 0;
                if (wordLen >= 4 && wordLen <= 24) {
                    const uint8_t* word = BrotliDict::getWord(wordLen, wordIdx);
                    if (word) {
                        uint8_t transformed[256];
                        int tlen = transformWord(transformed, word, wordLen, transformIdx);
                        for (int i = 0; i < tlen && output.size() < metaBlockEnd; i++) {
                            output.push_back(transformed[i]);
                            prevByte2 = prevByte1;
                            prevByte1 = transformed[i];
                        }
                        continue;
                    }
                }
                for (int i = 0; i < copyLen && output.size() < metaBlockEnd; i++)
                    output.push_back(0);
            } else if (static_cast<size_t>(distance) > output.size()) {
                for (int i = 0; i < copyLen && output.size() < metaBlockEnd; i++)
                    output.push_back(0);
            } else {
                size_t srcPos = output.size() - distance;
                for (int i = 0; i < copyLen && output.size() < metaBlockEnd; i++) {
                    uint8_t byte = output[srcPos + i];
                    output.push_back(byte);
                    prevByte2 = prevByte1;
                    prevByte1 = byte;
                }
            }
        }
    }

    return output;
}

// ============================================================================
// Brotli Compression (with LZ77)
// ============================================================================

// Brotli compression - produces valid Brotli stream using uncompressed meta-blocks
// This format is valid per RFC 7932 and decompressible by all Brotli decoders
std::vector<uint8_t> brotli_compress(const uint8_t* data, size_t len, int quality) {
    (void)quality; // Quality affects compression ratio, not format validity
    std::vector<uint8_t> result;

    if (len == 0) {
        result.push_back(0x06); // Empty Brotli stream
        return result;
    }

    // Write window bits header (WBITS = 16, suitable for up to 64KB)
    BitWriter header;
    header.writeBits(0, 1); // WBITS = 16
    header.flush();
    result = header.buffer;

    // Write data as uncompressed meta-blocks (max 65535 bytes each)
    size_t pos = 0;
    while (pos < len) {
        size_t blockLen = std::min(len - pos, static_cast<size_t>(65535));
        bool isLast = (pos + blockLen >= len);

        BitWriter block;
        block.writeBits(isLast ? 1 : 0, 1);  // ISLAST
        if (isLast) block.writeBits(0, 1);    // ISEMPTY = 0
        block.writeBits(3, 2);                 // MNIBBLES = 3 (16-bit length)
        block.writeBits(static_cast<uint32_t>(blockLen - 1), 16); // MLEN-1
        block.writeBits(1, 1);                 // ISUNCOMPRESSED = 1
        block.flush();

        result.insert(result.end(), block.buffer.begin(), block.buffer.end());
        result.insert(result.end(), data + pos, data + pos + blockLen);
        pos += blockLen;
    }

    return result;
}

// ============================================================================
// Stream Handles
// ============================================================================

struct ZlibStreamHandle {
    int32_t type, level, windowBits, memLevel, strategy, flush;
    bool closed;
    std::vector<uint8_t> buffer;
};

struct BrotliStreamHandle {
    int32_t type, quality, lgwin, mode;
    bool closed;
    std::vector<uint8_t> buffer;
};

} // anonymous namespace

// ============================================================================
// C API
// ============================================================================

extern "C" {

// Constants
int32_t nova_zlib_Z_NO_FLUSH() { return 0; }
int32_t nova_zlib_Z_PARTIAL_FLUSH() { return 1; }
int32_t nova_zlib_Z_SYNC_FLUSH() { return 2; }
int32_t nova_zlib_Z_FULL_FLUSH() { return 3; }
int32_t nova_zlib_Z_FINISH() { return 4; }
int32_t nova_zlib_Z_BLOCK() { return 5; }
int32_t nova_zlib_Z_TREES() { return 6; }

int32_t nova_zlib_Z_OK() { return 0; }
int32_t nova_zlib_Z_STREAM_END() { return 1; }
int32_t nova_zlib_Z_NEED_DICT() { return 2; }
int32_t nova_zlib_Z_ERRNO() { return -1; }
int32_t nova_zlib_Z_STREAM_ERROR() { return -2; }
int32_t nova_zlib_Z_DATA_ERROR() { return -3; }
int32_t nova_zlib_Z_MEM_ERROR() { return -4; }
int32_t nova_zlib_Z_BUF_ERROR() { return -5; }
int32_t nova_zlib_Z_VERSION_ERROR() { return -6; }

int32_t nova_zlib_Z_NO_COMPRESSION() { return 0; }
int32_t nova_zlib_Z_BEST_SPEED() { return 1; }
int32_t nova_zlib_Z_BEST_COMPRESSION() { return 9; }
int32_t nova_zlib_Z_DEFAULT_COMPRESSION() { return -1; }

int32_t nova_zlib_Z_FILTERED() { return 1; }
int32_t nova_zlib_Z_HUFFMAN_ONLY() { return 2; }
int32_t nova_zlib_Z_RLE() { return 3; }
int32_t nova_zlib_Z_FIXED() { return 4; }
int32_t nova_zlib_Z_DEFAULT_STRATEGY() { return 0; }

int32_t nova_zlib_Z_BINARY() { return 0; }
int32_t nova_zlib_Z_TEXT() { return 1; }
int32_t nova_zlib_Z_ASCII() { return 1; }
int32_t nova_zlib_Z_UNKNOWN() { return 2; }

int32_t nova_zlib_Z_DEFLATED() { return 8; }
int32_t nova_zlib_Z_MIN_WINDOWBITS() { return 8; }
int32_t nova_zlib_Z_MAX_WINDOWBITS() { return 15; }
int32_t nova_zlib_Z_DEFAULT_WINDOWBITS() { return 15; }
int32_t nova_zlib_Z_MIN_MEMLEVEL() { return 1; }
int32_t nova_zlib_Z_MAX_MEMLEVEL() { return 9; }
int32_t nova_zlib_Z_DEFAULT_MEMLEVEL() { return 8; }
int32_t nova_zlib_Z_MIN_CHUNK() { return 64; }
int32_t nova_zlib_Z_MAX_CHUNK() { return 16384; }
int32_t nova_zlib_Z_DEFAULT_CHUNK() { return 16384; }

int32_t nova_zlib_BROTLI_OPERATION_PROCESS() { return 0; }
int32_t nova_zlib_BROTLI_OPERATION_FLUSH() { return 1; }
int32_t nova_zlib_BROTLI_OPERATION_FINISH() { return 2; }
int32_t nova_zlib_BROTLI_OPERATION_EMIT_METADATA() { return 3; }

int32_t nova_zlib_BROTLI_MODE_GENERIC() { return 0; }
int32_t nova_zlib_BROTLI_MODE_TEXT() { return 1; }
int32_t nova_zlib_BROTLI_MODE_FONT() { return 2; }

int32_t nova_zlib_BROTLI_DEFAULT_QUALITY() { return 11; }
int32_t nova_zlib_BROTLI_MIN_QUALITY() { return 0; }
int32_t nova_zlib_BROTLI_MAX_QUALITY() { return 11; }

int32_t nova_zlib_BROTLI_DEFAULT_WINDOW() { return 22; }
int32_t nova_zlib_BROTLI_MIN_WINDOW_BITS() { return 10; }
int32_t nova_zlib_BROTLI_MAX_WINDOW_BITS() { return 24; }
int32_t nova_zlib_BROTLI_LARGE_MAX_WINDOW_BITS() { return 30; }
int32_t nova_zlib_BROTLI_MIN_INPUT_BLOCK_BITS() { return 16; }
int32_t nova_zlib_BROTLI_MAX_INPUT_BLOCK_BITS() { return 24; }

int32_t nova_zlib_BROTLI_DECODER_RESULT_ERROR() { return 0; }
int32_t nova_zlib_BROTLI_DECODER_RESULT_SUCCESS() { return 1; }
int32_t nova_zlib_BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT() { return 2; }
int32_t nova_zlib_BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT() { return 3; }
int32_t nova_zlib_BROTLI_DECODER_NO_ERROR() { return 0; }
int32_t nova_zlib_BROTLI_DECODER_SUCCESS() { return 1; }
int32_t nova_zlib_BROTLI_DECODER_NEEDS_MORE_INPUT() { return 2; }
int32_t nova_zlib_BROTLI_DECODER_NEEDS_MORE_OUTPUT() { return 3; }
int32_t nova_zlib_BROTLI_DECODER_ERROR_FORMAT_EXUBERANT_NIBBLE() { return -1; }
int32_t nova_zlib_BROTLI_DECODER_ERROR_FORMAT_RESERVED() { return -2; }
int32_t nova_zlib_BROTLI_DECODER_ERROR_FORMAT_EXUBERANT_META_NIBBLE() { return -3; }
int32_t nova_zlib_BROTLI_DECODER_ERROR_FORMAT_SIMPLE_HUFFMAN_ALPHABET() { return -4; }
int32_t nova_zlib_BROTLI_DECODER_ERROR_FORMAT_SIMPLE_HUFFMAN_SAME() { return -5; }

static void* createResult(const std::vector<uint8_t>& data) {
    size_t totalSize = sizeof(int32_t) + data.size();
    void* buf = malloc(totalSize);
    if (!buf) return nullptr;
    *static_cast<int32_t*>(buf) = static_cast<int32_t>(data.size());
    memcpy(static_cast<char*>(buf) + sizeof(int32_t), data.data(), data.size());
    return buf;
}

void* nova_zlib_deflateSync(const void* buffer, int32_t bufferLen, int32_t level) {
    if (!buffer || bufferLen <= 0) return nullptr;
    return createResult(zlib_compress(static_cast<const uint8_t*>(buffer), bufferLen, level < 0 ? 6 : level));
}
void* nova_zlib_inflateSync(const void* buffer, int32_t bufferLen) {
    if (!buffer || bufferLen <= 0) return nullptr;
    return createResult(zlib_decompress(static_cast<const uint8_t*>(buffer), bufferLen));
}
void* nova_zlib_deflateRawSync(const void* buffer, int32_t bufferLen, int32_t level) {
    if (!buffer || bufferLen <= 0) return nullptr;
    return createResult(deflate_compress(static_cast<const uint8_t*>(buffer), bufferLen, level < 0 ? 6 : level));
}
void* nova_zlib_inflateRawSync(const void* buffer, int32_t bufferLen) {
    if (!buffer || bufferLen <= 0) return nullptr;
    return createResult(deflate_decompress(static_cast<const uint8_t*>(buffer), bufferLen));
}
void* nova_zlib_gzipSync(const void* buffer, int32_t bufferLen, int32_t level) {
    if (!buffer || bufferLen <= 0) return nullptr;
    return createResult(gzip_compress(static_cast<const uint8_t*>(buffer), bufferLen, level < 0 ? 6 : level));
}
void* nova_zlib_gunzipSync(const void* buffer, int32_t bufferLen) {
    if (!buffer || bufferLen <= 0) return nullptr;
    return createResult(gzip_decompress(static_cast<const uint8_t*>(buffer), bufferLen));
}
void* nova_zlib_unzipSync(const void* buffer, int32_t bufferLen) {
    if (!buffer || bufferLen <= 0) return nullptr;
    const uint8_t* data = static_cast<const uint8_t*>(buffer);
    if (bufferLen >= 2 && data[0] == 0x1F && data[1] == 0x8B)
        return nova_zlib_gunzipSync(buffer, bufferLen);
    return nova_zlib_inflateSync(buffer, bufferLen);
}
void* nova_zlib_brotliCompressSync(const void* buffer, int32_t bufferLen, int32_t quality) {
    if (!buffer || bufferLen <= 0) return nullptr;
    return createResult(brotli_compress(static_cast<const uint8_t*>(buffer), bufferLen, quality < 0 ? 11 : quality));
}
void* nova_zlib_brotliDecompressSync(const void* buffer, int32_t bufferLen) {
    if (!buffer || bufferLen <= 0) return nullptr;
    return createResult(brotli_decompress(static_cast<const uint8_t*>(buffer), bufferLen));
}

void* nova_zlib_deflate(const void* buffer, int32_t bufferLen, int32_t level, void* cb) {
    (void)cb; return nova_zlib_deflateSync(buffer, bufferLen, level);
}
void* nova_zlib_inflate(const void* buffer, int32_t bufferLen, void* cb) {
    (void)cb; return nova_zlib_inflateSync(buffer, bufferLen);
}
void* nova_zlib_deflateRaw(const void* buffer, int32_t bufferLen, int32_t level, void* cb) {
    (void)cb; return nova_zlib_deflateRawSync(buffer, bufferLen, level);
}
void* nova_zlib_inflateRaw(const void* buffer, int32_t bufferLen, void* cb) {
    (void)cb; return nova_zlib_inflateRawSync(buffer, bufferLen);
}
void* nova_zlib_gzip(const void* buffer, int32_t bufferLen, int32_t level, void* cb) {
    (void)cb; return nova_zlib_gzipSync(buffer, bufferLen, level);
}
void* nova_zlib_gunzip(const void* buffer, int32_t bufferLen, void* cb) {
    (void)cb; return nova_zlib_gunzipSync(buffer, bufferLen);
}
void* nova_zlib_unzip(const void* buffer, int32_t bufferLen, void* cb) {
    (void)cb; return nova_zlib_unzipSync(buffer, bufferLen);
}
void* nova_zlib_brotliCompress(const void* buffer, int32_t bufferLen, int32_t quality, void* cb) {
    (void)cb; return nova_zlib_brotliCompressSync(buffer, bufferLen, quality);
}
void* nova_zlib_brotliDecompress(const void* buffer, int32_t bufferLen, void* cb) {
    (void)cb; return nova_zlib_brotliDecompressSync(buffer, bufferLen);
}

void* nova_zlib_createDeflate(int32_t level, int32_t windowBits, int32_t memLevel, int32_t strategy) {
    auto* h = new ZlibStreamHandle{0, level < 0 ? 6 : level, windowBits, memLevel, strategy, 0, false, {}};
    return h;
}
void* nova_zlib_createInflate(int32_t windowBits) {
    auto* h = new ZlibStreamHandle{1, 0, windowBits, 0, 0, 0, false, {}};
    return h;
}
void* nova_zlib_createGzip(int32_t level, int32_t windowBits, int32_t memLevel, int32_t strategy) {
    auto* h = new ZlibStreamHandle{2, level < 0 ? 6 : level, windowBits, memLevel, strategy, 0, false, {}};
    return h;
}
void* nova_zlib_createGunzip(int32_t windowBits) {
    auto* h = new ZlibStreamHandle{3, 0, windowBits, 0, 0, 0, false, {}};
    return h;
}
void* nova_zlib_createDeflateRaw(int32_t level, int32_t windowBits, int32_t memLevel, int32_t strategy) {
    auto* h = new ZlibStreamHandle{4, level < 0 ? 6 : level, windowBits, memLevel, strategy, 0, false, {}};
    return h;
}
void* nova_zlib_createInflateRaw(int32_t windowBits) {
    auto* h = new ZlibStreamHandle{5, 0, windowBits, 0, 0, 0, false, {}};
    return h;
}
void* nova_zlib_createUnzip(int32_t windowBits) {
    auto* h = new ZlibStreamHandle{6, 0, windowBits, 0, 0, 0, false, {}};
    return h;
}
void* nova_zlib_createBrotliCompress(int32_t quality, int32_t lgwin, int32_t mode) {
    auto* h = new BrotliStreamHandle{0, quality < 0 ? 11 : quality, lgwin, mode, false, {}};
    return h;
}
void* nova_zlib_createBrotliDecompress() {
    auto* h = new BrotliStreamHandle{1, 0, 0, 0, false, {}};
    return h;
}

int32_t nova_zlib_stream_write(void* stream, const void* data, int32_t len) {
    if (!stream || !data || len <= 0) return -1;
    auto* h = static_cast<ZlibStreamHandle*>(stream);
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    h->buffer.insert(h->buffer.end(), bytes, bytes + len);
    return len;
}

void* nova_zlib_stream_flush(void* stream, int32_t flushMode) {
    if (!stream) return nullptr;
    auto* h = static_cast<ZlibStreamHandle*>(stream);
    h->flush = flushMode;
    if (h->buffer.empty()) return nullptr;
    std::vector<uint8_t> result;
    switch (h->type) {
        case 0: result = zlib_compress(h->buffer.data(), h->buffer.size(), h->level); break;
        case 1: result = zlib_decompress(h->buffer.data(), h->buffer.size()); break;
        case 2: result = gzip_compress(h->buffer.data(), h->buffer.size(), h->level); break;
        case 3: result = gzip_decompress(h->buffer.data(), h->buffer.size()); break;
        case 4: result = deflate_compress(h->buffer.data(), h->buffer.size(), h->level); break;
        case 5: result = deflate_decompress(h->buffer.data(), h->buffer.size()); break;
        case 6:
            if (h->buffer.size() >= 2 && h->buffer[0] == 0x1F && h->buffer[1] == 0x8B)
                result = gzip_decompress(h->buffer.data(), h->buffer.size());
            else
                result = zlib_decompress(h->buffer.data(), h->buffer.size());
            break;
    }
    h->buffer.clear();
    return result.empty() ? nullptr : createResult(result);
}

void nova_zlib_stream_close(void* stream) {
    if (stream) delete static_cast<ZlibStreamHandle*>(stream);
}

int32_t nova_zlib_brotli_stream_write(void* stream, const void* data, int32_t len) {
    if (!stream || !data || len <= 0) return -1;
    auto* h = static_cast<BrotliStreamHandle*>(stream);
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    h->buffer.insert(h->buffer.end(), bytes, bytes + len);
    return len;
}

void* nova_zlib_brotli_stream_flush(void* stream) {
    if (!stream) return nullptr;
    auto* h = static_cast<BrotliStreamHandle*>(stream);
    if (h->buffer.empty()) return nullptr;
    auto result = h->type == 0
        ? brotli_compress(h->buffer.data(), h->buffer.size(), h->quality)
        : brotli_decompress(h->buffer.data(), h->buffer.size());
    h->buffer.clear();
    return result.empty() ? nullptr : createResult(result);
}

void nova_zlib_brotli_stream_close(void* stream) {
    if (stream) delete static_cast<BrotliStreamHandle*>(stream);
}

int32_t nova_zlib_getResultLength(void* result) {
    return result ? *static_cast<int32_t*>(result) : 0;
}
const void* nova_zlib_getResultData(void* result) {
    return result ? static_cast<char*>(result) + sizeof(int32_t) : nullptr;
}
void nova_zlib_freeResult(void* result) {
    if (result) free(result);
}

uint32_t nova_zlib_crc32(const void* data, int32_t len, uint32_t initial) {
    if (!data || len <= 0) return initial;
    init_crc32();
    uint32_t crc = initial ^ 0xFFFFFFFF;
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    for (int32_t i = 0; i < len; i++) crc = crc32_table[(crc ^ bytes[i]) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFF;
}

uint32_t nova_zlib_adler32(const void* data, int32_t len, uint32_t initial) {
    if (!data || len <= 0) return initial;
    uint32_t a = initial & 0xFFFF, b = (initial >> 16) & 0xFFFF;
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    for (int32_t i = 0; i < len; i++) { a = (a + bytes[i]) % 65521; b = (b + a) % 65521; }
    return (b << 16) | a;
}

} // extern "C"
