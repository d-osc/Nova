// Nova Querystring Module - Node.js compatible querystring API
// Provides URL query string parsing and formatting

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <algorithm>
#include <functional>

// Default separators
static const char DEFAULT_SEP = '&';
static const char DEFAULT_EQ = '=';
static const int DEFAULT_MAX_KEYS = 1000;

// Helper: Check if character is unreserved (RFC 3986)
static bool isUnreserved(char c) {
    return (c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') ||
           c == '-' || c == '_' || c == '.' || c == '~';
}

// Helper: Convert hex char to int
static int hexToInt(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

// Helper: Convert int to hex char
static char intToHex(int n, bool uppercase = false) {
    if (n < 10) return '0' + n;
    return (uppercase ? 'A' : 'a') + (n - 10);
}

// Storage for parsed query string
struct ParsedQuery {
    std::map<std::string, std::vector<std::string>> params;
};

// Custom escape/unescape functions
static std::function<std::string(const std::string&)> customEscape = nullptr;
static std::function<std::string(const std::string&)> customUnescape = nullptr;

extern "C" {

// ============================================================================
// Core Functions
// ============================================================================

// querystring.escape(str) - URL encode a string
const char* nova_querystring_escape(const char* str) {
    static std::string result;

    if (!str) {
        result = "";
        return result.c_str();
    }

    // Use custom escape if set
    if (customEscape) {
        result = customEscape(str);
        return result.c_str();
    }

    result = "";
    std::string input(str);

    for (unsigned char c : input) {
        if (isUnreserved(c)) {
            result += c;
        } else if (c == ' ') {
            result += '+';  // Query string encodes space as +
        } else {
            result += '%';
            result += intToHex((c >> 4) & 0x0F, true);
            result += intToHex(c & 0x0F, true);
        }
    }

    return result.c_str();
}

// querystring.unescape(str) - URL decode a string
const char* nova_querystring_unescape(const char* str) {
    static std::string result;

    if (!str) {
        result = "";
        return result.c_str();
    }

    // Use custom unescape if set
    if (customUnescape) {
        result = customUnescape(str);
        return result.c_str();
    }

    result = "";
    std::string input(str);
    size_t i = 0;

    while (i < input.size()) {
        if (input[i] == '%' && i + 2 < input.size()) {
            int high = hexToInt(input[i + 1]);
            int low = hexToInt(input[i + 2]);

            if (high >= 0 && low >= 0) {
                result += static_cast<char>((high << 4) | low);
                i += 3;
                continue;
            }
        } else if (input[i] == '+') {
            result += ' ';
            i++;
            continue;
        }

        result += input[i];
        i++;
    }

    return result.c_str();
}

// querystring.parse(str, sep, eq, options) - Parse query string to object
void* nova_querystring_parse(const char* str, const char* sep, const char* eq, int maxKeys) {
    auto* parsed = new ParsedQuery();

    if (!str || strlen(str) == 0) {
        return parsed;
    }

    char separator = (sep && strlen(sep) > 0) ? sep[0] : DEFAULT_SEP;
    char equals = (eq && strlen(eq) > 0) ? eq[0] : DEFAULT_EQ;
    int limit = (maxKeys > 0) ? maxKeys : DEFAULT_MAX_KEYS;

    std::string input(str);
    size_t start = 0;
    int keyCount = 0;

    while (start < input.size() && (limit == 0 || keyCount < limit)) {
        // Find next separator
        size_t sepPos = input.find(separator, start);
        std::string pair;

        if (sepPos == std::string::npos) {
            pair = input.substr(start);
            start = input.size();
        } else {
            pair = input.substr(start, sepPos - start);
            start = sepPos + 1;
        }

        if (pair.empty()) continue;

        // Find equals sign
        size_t eqPos = pair.find(equals);
        std::string key, value;

        if (eqPos == std::string::npos) {
            key = nova_querystring_unescape(pair.c_str());
            value = "";
        } else {
            key = nova_querystring_unescape(pair.substr(0, eqPos).c_str());
            value = nova_querystring_unescape(pair.substr(eqPos + 1).c_str());
        }

        if (!key.empty()) {
            parsed->params[key].push_back(value);
            keyCount++;
        }
    }

    return parsed;
}

// querystring.decode - Alias for parse
void* nova_querystring_decode(const char* str, const char* sep, const char* eq, int maxKeys) {
    return nova_querystring_parse(str, sep, eq, maxKeys);
}

// Get keys from parsed query
const char** nova_querystring_keys(void* parsed, int* count) {
    static std::vector<const char*> keys;
    static std::vector<std::string> keyStorage;

    auto* p = static_cast<ParsedQuery*>(parsed);
    keys.clear();
    keyStorage.clear();

    for (const auto& pair : p->params) {
        keyStorage.push_back(pair.first);
    }

    for (const auto& key : keyStorage) {
        keys.push_back(key.c_str());
    }

    *count = static_cast<int>(keys.size());
    return keys.data();
}

// Get value(s) for a key from parsed query
const char** nova_querystring_get(void* parsed, const char* key, int* count) {
    static std::vector<const char*> values;
    static std::vector<std::string> valueStorage;

    auto* p = static_cast<ParsedQuery*>(parsed);
    values.clear();
    valueStorage.clear();

    auto it = p->params.find(key);
    if (it != p->params.end()) {
        valueStorage = it->second;
        for (const auto& val : valueStorage) {
            values.push_back(val.c_str());
        }
    }

    *count = static_cast<int>(values.size());
    return values.data();
}

// Get first value for a key
const char* nova_querystring_getFirst(void* parsed, const char* key) {
    static std::string result;

    auto* p = static_cast<ParsedQuery*>(parsed);
    auto it = p->params.find(key);

    if (it != p->params.end() && !it->second.empty()) {
        result = it->second[0];
        return result.c_str();
    }

    return nullptr;
}

// Check if key exists
bool nova_querystring_has(void* parsed, const char* key) {
    auto* p = static_cast<ParsedQuery*>(parsed);
    return p->params.find(key) != p->params.end();
}

// Free parsed query
void nova_querystring_free(void* parsed) {
    delete static_cast<ParsedQuery*>(parsed);
}

// querystring.stringify(obj, sep, eq, options) - Convert object to query string
// Takes arrays of key-value pairs
const char* nova_querystring_stringify(const char** keys, const char** values, int count,
                                        const char* sep, const char* eq) {
    static std::string result;
    result = "";

    if (!keys || !values || count <= 0) {
        return result.c_str();
    }

    std::string separator = sep ? sep : "&";
    std::string equals = eq ? eq : "=";

    bool first = true;
    for (int i = 0; i < count; i++) {
        if (!keys[i]) continue;

        if (!first) {
            result += separator;
        }
        first = false;

        result += nova_querystring_escape(keys[i]);
        result += equals;

        if (values[i]) {
            result += nova_querystring_escape(values[i]);
        }
    }

    return result.c_str();
}

// querystring.encode - Alias for stringify
const char* nova_querystring_encode(const char** keys, const char** values, int count,
                                     const char* sep, const char* eq) {
    return nova_querystring_stringify(keys, values, count, sep, eq);
}

// Stringify from parsed object
const char* nova_querystring_stringifyParsed(void* parsed, const char* sep, const char* eq) {
    static std::string result;
    result = "";

    auto* p = static_cast<ParsedQuery*>(parsed);

    std::string separator = sep ? sep : "&";
    std::string equals = eq ? eq : "=";

    bool first = true;
    for (const auto& pair : p->params) {
        for (const auto& value : pair.second) {
            if (!first) {
                result += separator;
            }
            first = false;

            result += nova_querystring_escape(pair.first.c_str());
            result += equals;
            result += nova_querystring_escape(value.c_str());
        }
    }

    return result.c_str();
}

// ============================================================================
// Custom Escape/Unescape Functions
// ============================================================================

// Set custom escape function
void nova_querystring_setEscape(const char* (*escapeFunc)(const char*)) {
    if (escapeFunc) {
        customEscape = [escapeFunc](const std::string& s) -> std::string {
            const char* result = escapeFunc(s.c_str());
            return result ? std::string(result) : "";
        };
    } else {
        customEscape = nullptr;
    }
}

// Set custom unescape function
void nova_querystring_setUnescape(const char* (*unescapeFunc)(const char*)) {
    if (unescapeFunc) {
        customUnescape = [unescapeFunc](const std::string& s) -> std::string {
            const char* result = unescapeFunc(s.c_str());
            return result ? std::string(result) : "";
        };
    } else {
        customUnescape = nullptr;
    }
}

// Reset to default escape/unescape
void nova_querystring_resetEscape() {
    customEscape = nullptr;
}

void nova_querystring_resetUnescape() {
    customUnescape = nullptr;
}

// ============================================================================
// Utility Functions
// ============================================================================

// Parse to JSON string (for easy interop)
const char* nova_querystring_parseToJSON(const char* str, const char* sep, const char* eq, int maxKeys) {
    static std::string result;

    void* parsed = nova_querystring_parse(str, sep, eq, maxKeys);
    auto* p = static_cast<ParsedQuery*>(parsed);

    result = "{";
    bool first = true;

    for (const auto& pair : p->params) {
        if (!first) result += ",";
        first = false;

        // Escape key for JSON
        result += "\"";
        for (char c : pair.first) {
            if (c == '"') result += "\\\"";
            else if (c == '\\') result += "\\\\";
            else if (c == '\n') result += "\\n";
            else if (c == '\r') result += "\\r";
            else if (c == '\t') result += "\\t";
            else result += c;
        }
        result += "\":";

        // Value(s)
        if (pair.second.size() == 1) {
            // Single value - output as string
            result += "\"";
            for (char c : pair.second[0]) {
                if (c == '"') result += "\\\"";
                else if (c == '\\') result += "\\\\";
                else if (c == '\n') result += "\\n";
                else if (c == '\r') result += "\\r";
                else if (c == '\t') result += "\\t";
                else result += c;
            }
            result += "\"";
        } else {
            // Multiple values - output as array
            result += "[";
            bool firstVal = true;
            for (const auto& val : pair.second) {
                if (!firstVal) result += ",";
                firstVal = false;

                result += "\"";
                for (char c : val) {
                    if (c == '"') result += "\\\"";
                    else if (c == '\\') result += "\\\\";
                    else if (c == '\n') result += "\\n";
                    else if (c == '\r') result += "\\r";
                    else if (c == '\t') result += "\\t";
                    else result += c;
                }
                result += "\"";
            }
            result += "]";
        }
    }

    result += "}";

    nova_querystring_free(parsed);
    return result.c_str();
}

// Count parameters in query string
int nova_querystring_count(const char* str, const char* sep) {
    if (!str || strlen(str) == 0) return 0;

    char separator = (sep && strlen(sep) > 0) ? sep[0] : DEFAULT_SEP;
    std::string input(str);

    int count = 1;
    for (char c : input) {
        if (c == separator) count++;
    }

    return count;
}

// Check if string is valid query string format
bool nova_querystring_isValid(const char* str) {
    if (!str) return true;  // Empty is valid

    std::string input(str);

    for (size_t i = 0; i < input.size(); i++) {
        char c = input[i];

        // Check for invalid percent encoding
        if (c == '%') {
            if (i + 2 >= input.size()) return false;
            if (hexToInt(input[i + 1]) < 0) return false;
            if (hexToInt(input[i + 2]) < 0) return false;
        }
    }

    return true;
}

// Get default separator
char nova_querystring_defaultSep() {
    return DEFAULT_SEP;
}

// Get default equals sign
char nova_querystring_defaultEq() {
    return DEFAULT_EQ;
}

// Get default max keys
int nova_querystring_defaultMaxKeys() {
    return DEFAULT_MAX_KEYS;
}

// ============================================================================
// Parsed Query Object Manipulation
// ============================================================================

// Create new empty parsed query
void* nova_querystring_create() {
    return new ParsedQuery();
}

// Add key-value pair to parsed query
void nova_querystring_set(void* parsed, const char* key, const char* value) {
    auto* p = static_cast<ParsedQuery*>(parsed);
    if (key) {
        p->params[key].clear();
        p->params[key].push_back(value ? value : "");
    }
}

// Append value to existing key
void nova_querystring_append(void* parsed, const char* key, const char* value) {
    auto* p = static_cast<ParsedQuery*>(parsed);
    if (key) {
        p->params[key].push_back(value ? value : "");
    }
}

// Delete key from parsed query
void nova_querystring_delete(void* parsed, const char* key) {
    auto* p = static_cast<ParsedQuery*>(parsed);
    if (key) {
        p->params.erase(key);
    }
}

// Clear all entries
void nova_querystring_clear(void* parsed) {
    auto* p = static_cast<ParsedQuery*>(parsed);
    p->params.clear();
}

// Get number of unique keys
int nova_querystring_size(void* parsed) {
    auto* p = static_cast<ParsedQuery*>(parsed);
    return static_cast<int>(p->params.size());
}

// Iterate entries (returns key, sets value count)
const char* nova_querystring_iterate(void* parsed, int index, int* valueCount) {
    static std::string key;

    auto* p = static_cast<ParsedQuery*>(parsed);

    if (index < 0 || index >= static_cast<int>(p->params.size())) {
        *valueCount = 0;
        return nullptr;
    }

    auto it = p->params.begin();
    std::advance(it, index);

    key = it->first;
    *valueCount = static_cast<int>(it->second.size());

    return key.c_str();
}

// Sort keys alphabetically
void nova_querystring_sort(void* parsed) {
    // std::map is already sorted by key
    (void)parsed;
}

// Merge two parsed queries
void nova_querystring_merge(void* dest, void* src) {
    auto* d = static_cast<ParsedQuery*>(dest);
    auto* s = static_cast<ParsedQuery*>(src);

    for (const auto& pair : s->params) {
        for (const auto& val : pair.second) {
            d->params[pair.first].push_back(val);
        }
    }
}

// Clone parsed query
void* nova_querystring_clone(void* parsed) {
    auto* p = static_cast<ParsedQuery*>(parsed);
    auto* clone = new ParsedQuery();
    clone->params = p->params;
    return clone;
}

// Cleanup
void nova_querystring_cleanup() {
    customEscape = nullptr;
    customUnescape = nullptr;
}

} // extern "C"
