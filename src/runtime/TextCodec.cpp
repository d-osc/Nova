// TextEncoder/TextDecoder Runtime Implementation for Nova Compiler
// Encoding API (Web APIs)

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

extern "C" {

// ============================================================================
// TextEncoder Structure
// ============================================================================

struct NovaTextEncoder {
    char* encoding;  // Always "utf-8"
};

// ============================================================================
// TextDecoder Structure
// ============================================================================

struct NovaTextDecoder {
    char* encoding;
    bool fatal;
    bool ignoreBOM;
};

// ============================================================================
// TextEncoder Constructor: new TextEncoder()
// ============================================================================

void* nova_textencoder_create() {
    NovaTextEncoder* encoder = new NovaTextEncoder();
    encoder->encoding = new char[6];
    strcpy(encoder->encoding, "utf-8");
    return encoder;
}

// ============================================================================
// TextEncoder.prototype.encoding (getter)
// Always returns "utf-8"
// ============================================================================

const char* nova_textencoder_get_encoding(void* encoderPtr) {
    if (!encoderPtr) return "utf-8";
    return static_cast<NovaTextEncoder*>(encoderPtr)->encoding;
}

// ============================================================================
// TextEncoder.prototype.encode(input)
// Encodes a string into a Uint8Array (returns as byte array)
// ============================================================================

struct NovaUint8ArrayResult {
    uint8_t* data;
    int64_t length;
};

void* nova_textencoder_encode(void* encoderPtr, const char* input) {
    (void)encoderPtr;  // Unused, always UTF-8

    if (!input) {
        NovaUint8ArrayResult* result = new NovaUint8ArrayResult();
        result->data = nullptr;
        result->length = 0;
        return result;
    }

    size_t len = strlen(input);
    NovaUint8ArrayResult* result = new NovaUint8ArrayResult();
    result->data = new uint8_t[len];
    result->length = (int64_t)len;

    // UTF-8 encoding (input is already UTF-8 in C++)
    memcpy(result->data, input, len);

    return result;
}

// ============================================================================
// TextEncoder.prototype.encodeInto(source, destination)
// Encodes into existing Uint8Array, returns { read, written }
// ============================================================================

struct NovaEncodeIntoResult {
    int64_t read;
    int64_t written;
};

void* nova_textencoder_encodeInto(void* encoderPtr, const char* source, void* destPtr, int64_t destLength) {
    (void)encoderPtr;

    NovaEncodeIntoResult* result = new NovaEncodeIntoResult();
    result->read = 0;
    result->written = 0;

    if (!source || !destPtr || destLength <= 0) {
        return result;
    }

    uint8_t* dest = static_cast<uint8_t*>(destPtr);
    size_t sourceLen = strlen(source);
    size_t bytesToWrite = (sourceLen < (size_t)destLength) ? sourceLen : (size_t)destLength;

    memcpy(dest, source, bytesToWrite);
    result->read = (int64_t)bytesToWrite;
    result->written = (int64_t)bytesToWrite;

    return result;
}

// ============================================================================
// TextEncoder Destructor
// ============================================================================

void nova_textencoder_destroy(void* encoderPtr) {
    if (!encoderPtr) return;

    NovaTextEncoder* encoder = static_cast<NovaTextEncoder*>(encoderPtr);
    delete[] encoder->encoding;
    delete encoder;
}

// ============================================================================
// TextDecoder Constructor: new TextDecoder(label?, options?)
// ============================================================================

void* nova_textdecoder_create() {
    NovaTextDecoder* decoder = new NovaTextDecoder();
    decoder->encoding = new char[6];
    strcpy(decoder->encoding, "utf-8");
    decoder->fatal = false;
    decoder->ignoreBOM = false;
    return decoder;
}

void* nova_textdecoder_create_with_encoding(const char* label) {
    NovaTextDecoder* decoder = new NovaTextDecoder();

    // Normalize encoding label
    std::string enc = label ? label : "utf-8";
    // Convert to lowercase
    for (char& c : enc) {
        c = tolower(c);
    }

    // Map common aliases
    if (enc == "utf8" || enc == "utf-8" || enc == "unicode-1-1-utf-8") {
        enc = "utf-8";
    } else if (enc == "ascii" || enc == "us-ascii") {
        enc = "ascii";
    } else if (enc == "utf-16" || enc == "utf-16le") {
        enc = "utf-16le";
    } else if (enc == "utf-16be") {
        enc = "utf-16be";
    } else if (enc == "iso-8859-1" || enc == "latin1" || enc == "latin-1") {
        enc = "iso-8859-1";
    }
    // Default to utf-8 for unsupported encodings
    else {
        enc = "utf-8";
    }

    decoder->encoding = new char[enc.length() + 1];
    strcpy(decoder->encoding, enc.c_str());
    decoder->fatal = false;
    decoder->ignoreBOM = false;

    return decoder;
}

void* nova_textdecoder_create_with_options(const char* label, int64_t fatal, int64_t ignoreBOM) {
    NovaTextDecoder* decoder = static_cast<NovaTextDecoder*>(
        nova_textdecoder_create_with_encoding(label)
    );
    decoder->fatal = fatal != 0;
    decoder->ignoreBOM = ignoreBOM != 0;
    return decoder;
}

// ============================================================================
// TextDecoder.prototype.encoding (getter)
// ============================================================================

const char* nova_textdecoder_get_encoding(void* decoderPtr) {
    if (!decoderPtr) return "utf-8";
    return static_cast<NovaTextDecoder*>(decoderPtr)->encoding;
}

// ============================================================================
// TextDecoder.prototype.fatal (getter)
// ============================================================================

int64_t nova_textdecoder_get_fatal(void* decoderPtr) {
    if (!decoderPtr) return 0;
    return static_cast<NovaTextDecoder*>(decoderPtr)->fatal ? 1 : 0;
}

// ============================================================================
// TextDecoder.prototype.ignoreBOM (getter)
// ============================================================================

int64_t nova_textdecoder_get_ignoreBOM(void* decoderPtr) {
    if (!decoderPtr) return 0;
    return static_cast<NovaTextDecoder*>(decoderPtr)->ignoreBOM ? 1 : 0;
}

// ============================================================================
// TextDecoder.prototype.decode(input?, options?)
// Decodes bytes into a string
// ============================================================================

const char* nova_textdecoder_decode(void* decoderPtr, void* inputPtr, int64_t inputLength) {
    if (!decoderPtr) return "";

    NovaTextDecoder* decoder = static_cast<NovaTextDecoder*>(decoderPtr);

    if (!inputPtr || inputLength <= 0) {
        return "";
    }

    uint8_t* input = static_cast<uint8_t*>(inputPtr);

    // Get encoding
    std::string enc = decoder->encoding;

    // Handle BOM for UTF-8
    size_t offset = 0;
    if (!decoder->ignoreBOM && inputLength >= 3) {
        if (input[0] == 0xEF && input[1] == 0xBB && input[2] == 0xBF) {
            offset = 3;  // Skip UTF-8 BOM
        }
    }

    // For UTF-8, just copy (already UTF-8)
    if (enc == "utf-8" || enc == "ascii") {
        static thread_local std::string result;
        result.assign(reinterpret_cast<char*>(input + offset), inputLength - offset);
        return result.c_str();
    }

    // For ISO-8859-1/Latin-1, convert to UTF-8
    if (enc == "iso-8859-1") {
        static thread_local std::string result;
        result.clear();
        for (size_t i = offset; i < (size_t)inputLength; i++) {
            uint8_t c = input[i];
            if (c < 0x80) {
                result += (char)c;
            } else {
                // Convert to UTF-8
                result += (char)(0xC0 | (c >> 6));
                result += (char)(0x80 | (c & 0x3F));
            }
        }
        return result.c_str();
    }

    // For UTF-16LE
    if (enc == "utf-16le") {
        static thread_local std::string result;
        result.clear();

        // Check for BOM
        if (!decoder->ignoreBOM && inputLength >= 2) {
            if (input[0] == 0xFF && input[1] == 0xFE) {
                offset = 2;
            }
        }

        for (size_t i = offset; i + 1 < (size_t)inputLength; i += 2) {
            uint16_t codepoint = input[i] | (input[i + 1] << 8);

            // Convert to UTF-8
            if (codepoint < 0x80) {
                result += (char)codepoint;
            } else if (codepoint < 0x800) {
                result += (char)(0xC0 | (codepoint >> 6));
                result += (char)(0x80 | (codepoint & 0x3F));
            } else {
                result += (char)(0xE0 | (codepoint >> 12));
                result += (char)(0x80 | ((codepoint >> 6) & 0x3F));
                result += (char)(0x80 | (codepoint & 0x3F));
            }
        }
        return result.c_str();
    }

    // For UTF-16BE
    if (enc == "utf-16be") {
        static thread_local std::string result;
        result.clear();

        // Check for BOM
        if (!decoder->ignoreBOM && inputLength >= 2) {
            if (input[0] == 0xFE && input[1] == 0xFF) {
                offset = 2;
            }
        }

        for (size_t i = offset; i + 1 < (size_t)inputLength; i += 2) {
            uint16_t codepoint = (input[i] << 8) | input[i + 1];

            // Convert to UTF-8
            if (codepoint < 0x80) {
                result += (char)codepoint;
            } else if (codepoint < 0x800) {
                result += (char)(0xC0 | (codepoint >> 6));
                result += (char)(0x80 | (codepoint & 0x3F));
            } else {
                result += (char)(0xE0 | (codepoint >> 12));
                result += (char)(0x80 | ((codepoint >> 6) & 0x3F));
                result += (char)(0x80 | (codepoint & 0x3F));
            }
        }
        return result.c_str();
    }

    // Default: treat as UTF-8
    static thread_local std::string result;
    result.assign(reinterpret_cast<char*>(input + offset), inputLength - offset);
    return result.c_str();
}

// ============================================================================
// TextDecoder Destructor
// ============================================================================

void nova_textdecoder_destroy(void* decoderPtr) {
    if (!decoderPtr) return;

    NovaTextDecoder* decoder = static_cast<NovaTextDecoder*>(decoderPtr);
    delete[] decoder->encoding;
    delete decoder;
}

// ============================================================================
// Utility: Get encoded bytes data and length
// ============================================================================

uint8_t* nova_uint8array_result_get_data(void* resultPtr) {
    if (!resultPtr) return nullptr;
    return static_cast<NovaUint8ArrayResult*>(resultPtr)->data;
}

int64_t nova_uint8array_result_get_length(void* resultPtr) {
    if (!resultPtr) return 0;
    return static_cast<NovaUint8ArrayResult*>(resultPtr)->length;
}

void nova_uint8array_result_destroy(void* resultPtr) {
    if (!resultPtr) return;
    NovaUint8ArrayResult* result = static_cast<NovaUint8ArrayResult*>(resultPtr);
    delete[] result->data;
    delete result;
}

// ============================================================================
// EncodeInto result getters
// ============================================================================

int64_t nova_encodeinto_result_get_read(void* resultPtr) {
    if (!resultPtr) return 0;
    return static_cast<NovaEncodeIntoResult*>(resultPtr)->read;
}

int64_t nova_encodeinto_result_get_written(void* resultPtr) {
    if (!resultPtr) return 0;
    return static_cast<NovaEncodeIntoResult*>(resultPtr)->written;
}

void nova_encodeinto_result_destroy(void* resultPtr) {
    if (!resultPtr) return;
    delete static_cast<NovaEncodeIntoResult*>(resultPtr);
}

} // extern "C"
