// Nova Punycode Module - Node.js compatible punycode API
// RFC 3492 compliant Punycode implementation

#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>

// Punycode parameters (RFC 3492)
static const uint32_t BASE = 36;
static const uint32_t TMIN = 1;
static const uint32_t TMAX = 26;
static const uint32_t SKEW = 38;
static const uint32_t DAMP = 700;
static const uint32_t INITIAL_BIAS = 72;
static const uint32_t INITIAL_N = 128;
static const char DELIMITER = '-';

// Punycode version
static const char* PUNYCODE_VERSION = "2.3.1";

// Helper: Adapt bias
static uint32_t adapt(uint32_t delta, uint32_t numPoints, bool firstTime) {
    delta = firstTime ? delta / DAMP : delta >> 1;
    delta += delta / numPoints;

    uint32_t k = 0;
    while (delta > ((BASE - TMIN) * TMAX) / 2) {
        delta /= BASE - TMIN;
        k += BASE;
    }

    return k + (((BASE - TMIN + 1) * delta) / (delta + SKEW));
}

// Helper: Decode a single digit
static uint32_t decodeDigit(uint32_t cp) {
    if (cp >= 'a' && cp <= 'z') return cp - 'a';
    if (cp >= 'A' && cp <= 'Z') return cp - 'A';
    if (cp >= '0' && cp <= '9') return cp - '0' + 26;
    return BASE; // Invalid
}

// Helper: Encode a single digit
static char encodeDigit(uint32_t d, bool uppercase) {
    if (d < 26) {
        return uppercase ? ('A' + d) : ('a' + d);
    }
    return '0' + (d - 26);
}

// Helper: Convert UTF-8 to code points
static std::vector<uint32_t> utf8ToCodePoints(const std::string& str) {
    std::vector<uint32_t> codePoints;
    size_t i = 0;

    while (i < str.size()) {
        uint32_t cp;
        unsigned char c = str[i];

        if ((c & 0x80) == 0) {
            // ASCII
            cp = c;
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            // 2-byte sequence
            cp = (c & 0x1F) << 6;
            if (i + 1 < str.size()) {
                cp |= (str[i + 1] & 0x3F);
            }
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            // 3-byte sequence
            cp = (c & 0x0F) << 12;
            if (i + 1 < str.size()) cp |= (str[i + 1] & 0x3F) << 6;
            if (i + 2 < str.size()) cp |= (str[i + 2] & 0x3F);
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            // 4-byte sequence
            cp = (c & 0x07) << 18;
            if (i + 1 < str.size()) cp |= (str[i + 1] & 0x3F) << 12;
            if (i + 2 < str.size()) cp |= (str[i + 2] & 0x3F) << 6;
            if (i + 3 < str.size()) cp |= (str[i + 3] & 0x3F);
            i += 4;
        } else {
            // Invalid UTF-8, skip
            i += 1;
            continue;
        }

        codePoints.push_back(cp);
    }

    return codePoints;
}

// Helper: Convert code points to UTF-8
static std::string codePointsToUtf8(const std::vector<uint32_t>& codePoints) {
    std::string result;

    for (uint32_t cp : codePoints) {
        if (cp < 0x80) {
            result += static_cast<char>(cp);
        } else if (cp < 0x800) {
            result += static_cast<char>(0xC0 | (cp >> 6));
            result += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            result += static_cast<char>(0xE0 | (cp >> 12));
            result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            result += static_cast<char>(0xF0 | (cp >> 18));
            result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }

    return result;
}

extern "C" {

// ============================================================================
// Core Punycode Functions
// ============================================================================

// punycode.decode(string) - Decode Punycode to Unicode
const char* nova_punycode_decode(const char* input) {
    static std::string result;

    if (!input) {
        result = "";
        return result.c_str();
    }

    std::string str(input);
    std::vector<uint32_t> output;

    // Find the last delimiter
    size_t delimPos = str.rfind(DELIMITER);

    // Copy basic code points (before delimiter)
    size_t basicLength = 0;
    if (delimPos != std::string::npos) {
        for (size_t i = 0; i < delimPos; i++) {
            output.push_back(static_cast<uint32_t>(str[i]));
        }
        basicLength = delimPos + 1;
    }

    // Main decoding loop
    uint32_t n = INITIAL_N;
    uint32_t bias = INITIAL_BIAS;
    uint32_t i = 0;

    for (size_t in = basicLength; in < str.size(); ) {
        uint32_t oldi = i;
        uint32_t w = 1;

        for (uint32_t k = BASE; ; k += BASE) {
            if (in >= str.size()) break;

            uint32_t digit = decodeDigit(str[in++]);
            if (digit >= BASE) break;

            i += digit * w;

            uint32_t t;
            if (k <= bias) t = TMIN;
            else if (k >= bias + TMAX) t = TMAX;
            else t = k - bias;

            if (digit < t) break;

            w *= BASE - t;
        }

        uint32_t outLen = static_cast<uint32_t>(output.size() + 1);
        bias = adapt(i - oldi, outLen, oldi == 0);
        n += i / outLen;
        i %= outLen;

        output.insert(output.begin() + i, n);
        i++;
    }

    result = codePointsToUtf8(output);
    return result.c_str();
}

// punycode.encode(string) - Encode Unicode to Punycode
const char* nova_punycode_encode(const char* input) {
    static std::string result;

    if (!input) {
        result = "";
        return result.c_str();
    }

    std::vector<uint32_t> codePoints = utf8ToCodePoints(input);
    result = "";

    // Copy basic code points
    uint32_t basicCount = 0;
    for (uint32_t cp : codePoints) {
        if (cp < 128) {
            result += static_cast<char>(cp);
            basicCount++;
        }
    }

    // Add delimiter if there were basic code points
    if (basicCount > 0) {
        result += DELIMITER;
    }

    // Main encoding loop
    uint32_t n = INITIAL_N;
    uint32_t delta = 0;
    uint32_t bias = INITIAL_BIAS;
    uint32_t h = basicCount;
    uint32_t inputLength = static_cast<uint32_t>(codePoints.size());

    while (h < inputLength) {
        // Find the minimum code point >= n
        uint32_t m = UINT32_MAX;
        for (uint32_t cp : codePoints) {
            if (cp >= n && cp < m) {
                m = cp;
            }
        }

        delta += (m - n) * (h + 1);
        n = m;

        for (uint32_t cp : codePoints) {
            if (cp < n) {
                delta++;
            } else if (cp == n) {
                uint32_t q = delta;

                for (uint32_t k = BASE; ; k += BASE) {
                    uint32_t t;
                    if (k <= bias) t = TMIN;
                    else if (k >= bias + TMAX) t = TMAX;
                    else t = k - bias;

                    if (q < t) break;

                    result += encodeDigit(t + (q - t) % (BASE - t), false);
                    q = (q - t) / (BASE - t);
                }

                result += encodeDigit(q, false);
                bias = adapt(delta, h + 1, h == basicCount);
                delta = 0;
                h++;
            }
        }

        delta++;
        n++;
    }

    return result.c_str();
}

// punycode.toASCII(domain) - Convert domain to ASCII (IDN encoding)
const char* nova_punycode_toASCII(const char* domain) {
    static std::string result;

    if (!domain) {
        result = "";
        return result.c_str();
    }

    std::string input(domain);
    result = "";

    // Split by dots and encode each label
    size_t start = 0;
    size_t pos;

    while ((pos = input.find('.', start)) != std::string::npos) {
        std::string label = input.substr(start, pos - start);

        // Check if label contains non-ASCII
        bool hasNonAscii = false;
        for (char c : label) {
            if (static_cast<unsigned char>(c) >= 128) {
                hasNonAscii = true;
                break;
            }
        }

        if (hasNonAscii) {
            result += "xn--";
            result += nova_punycode_encode(label.c_str());
        } else {
            result += label;
        }
        result += '.';

        start = pos + 1;
    }

    // Process the last label
    std::string label = input.substr(start);
    bool hasNonAscii = false;
    for (char c : label) {
        if (static_cast<unsigned char>(c) >= 128) {
            hasNonAscii = true;
            break;
        }
    }

    if (hasNonAscii) {
        result += "xn--";
        result += nova_punycode_encode(label.c_str());
    } else {
        result += label;
    }

    return result.c_str();
}

// punycode.toUnicode(domain) - Convert ASCII domain to Unicode
const char* nova_punycode_toUnicode(const char* domain) {
    static std::string result;

    if (!domain) {
        result = "";
        return result.c_str();
    }

    std::string input(domain);
    result = "";

    // Split by dots and decode each label
    size_t start = 0;
    size_t pos;

    while ((pos = input.find('.', start)) != std::string::npos) {
        std::string label = input.substr(start, pos - start);

        // Check if label is Punycode encoded
        if (label.size() > 4 &&
            (label.substr(0, 4) == "xn--" || label.substr(0, 4) == "XN--")) {
            result += nova_punycode_decode(label.substr(4).c_str());
        } else {
            result += label;
        }
        result += '.';

        start = pos + 1;
    }

    // Process the last label
    std::string label = input.substr(start);
    if (label.size() > 4 &&
        (label.substr(0, 4) == "xn--" || label.substr(0, 4) == "XN--")) {
        result += nova_punycode_decode(label.substr(4).c_str());
    } else {
        result += label;
    }

    return result.c_str();
}

// ============================================================================
// UCS-2 Functions
// ============================================================================

// punycode.ucs2.decode(string) - Convert string to array of code points
uint32_t* nova_punycode_ucs2_decode(const char* input, int* length) {
    static std::vector<uint32_t> codePoints;

    if (!input) {
        *length = 0;
        return nullptr;
    }

    codePoints = utf8ToCodePoints(input);
    *length = static_cast<int>(codePoints.size());

    return codePoints.data();
}

// punycode.ucs2.encode(codePoints) - Convert array of code points to string
const char* nova_punycode_ucs2_encode(const uint32_t* codePoints, int length) {
    static std::string result;

    if (!codePoints || length <= 0) {
        result = "";
        return result.c_str();
    }

    std::vector<uint32_t> cp(codePoints, codePoints + length);
    result = codePointsToUtf8(cp);

    return result.c_str();
}

// ============================================================================
// Version and Utilities
// ============================================================================

// punycode.version
const char* nova_punycode_version() {
    return PUNYCODE_VERSION;
}

// Check if string contains non-ASCII characters
bool nova_punycode_isNonASCII(const char* str) {
    if (!str) return false;

    while (*str) {
        if (static_cast<unsigned char>(*str) >= 128) {
            return true;
        }
        str++;
    }
    return false;
}

// Check if domain is Punycode encoded
bool nova_punycode_isPunycode(const char* str) {
    if (!str) return false;

    std::string s(str);

    // Check for xn-- prefix in any label
    size_t pos = 0;
    while (pos < s.size()) {
        size_t dot = s.find('.', pos);
        std::string label = (dot == std::string::npos) ?
                           s.substr(pos) : s.substr(pos, dot - pos);

        if (label.size() > 4 &&
            (label.substr(0, 4) == "xn--" || label.substr(0, 4) == "XN--")) {
            return true;
        }

        if (dot == std::string::npos) break;
        pos = dot + 1;
    }

    return false;
}

// Convert a single code point to string
const char* nova_punycode_codePointToString(uint32_t codePoint) {
    static std::string result;
    std::vector<uint32_t> cp = {codePoint};
    result = codePointsToUtf8(cp);
    return result.c_str();
}

// Get code point at index
uint32_t nova_punycode_codePointAt(const char* str, int index) {
    if (!str) return 0;

    std::vector<uint32_t> codePoints = utf8ToCodePoints(str);

    if (index < 0 || index >= static_cast<int>(codePoints.size())) {
        return 0;
    }

    return codePoints[index];
}

// Get string length in code points
int nova_punycode_codePointLength(const char* str) {
    if (!str) return 0;
    return static_cast<int>(utf8ToCodePoints(str).size());
}

// ============================================================================
// Low-level encoding/decoding helpers
// ============================================================================

// Encode a single digit (0-35) to character
char nova_punycode_digitToChar(int digit, bool uppercase) {
    return encodeDigit(digit, uppercase);
}

// Decode a character to digit (0-35)
int nova_punycode_charToDigit(char c) {
    return static_cast<int>(decodeDigit(static_cast<uint32_t>(c)));
}

// Get the Punycode delimiter character
char nova_punycode_delimiter() {
    return DELIMITER;
}

// Get base value
int nova_punycode_base() {
    return BASE;
}

// Get tmin value
int nova_punycode_tmin() {
    return TMIN;
}

// Get tmax value
int nova_punycode_tmax() {
    return TMAX;
}

// Get skew value
int nova_punycode_skew() {
    return SKEW;
}

// Get damp value
int nova_punycode_damp() {
    return DAMP;
}

// Get initial bias
int nova_punycode_initialBias() {
    return INITIAL_BIAS;
}

// Get initial N
int nova_punycode_initialN() {
    return INITIAL_N;
}

// Cleanup (placeholder for consistency)
void nova_punycode_cleanup() {
    // Nothing to clean up
}

} // extern "C"
