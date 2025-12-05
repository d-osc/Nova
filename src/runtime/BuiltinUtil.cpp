// Nova Builtin Util Module Implementation
// Provides Node.js-compatible util API

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <set>
#include <regex>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <unistd.h>
#endif

extern "C" {

// ============================================================================
// util.format(format, ...args)
// ============================================================================

const char* nova_util_format(const char* fmt, const char** args, int argCount) {
    if (!fmt) return "";

    static thread_local std::string result;
    result.clear();

    int argIndex = 0;
    const char* p = fmt;

    while (*p) {
        if (*p == '%' && *(p + 1)) {
            char spec = *(p + 1);
            if (argIndex < argCount && args[argIndex]) {
                switch (spec) {
                    case 's':  // String
                        result += args[argIndex];
                        argIndex++;
                        p += 2;
                        continue;
                    case 'd':  // Number (integer)
                    case 'i':
                        result += args[argIndex];
                        argIndex++;
                        p += 2;
                        continue;
                    case 'f':  // Float
                        result += args[argIndex];
                        argIndex++;
                        p += 2;
                        continue;
                    case 'j':  // JSON
                        result += args[argIndex];
                        argIndex++;
                        p += 2;
                        continue;
                    case 'o':  // Object
                    case 'O':
                        result += args[argIndex];
                        argIndex++;
                        p += 2;
                        continue;
                    case '%':  // Literal %
                        result += '%';
                        p += 2;
                        continue;
                    default:
                        break;
                }
            }
        }
        result += *p++;
    }

    // Append remaining args
    while (argIndex < argCount) {
        result += ' ';
        if (args[argIndex]) {
            result += args[argIndex];
        }
        argIndex++;
    }

    return result.c_str();
}

// Simple format with single argument
const char* nova_util_format1(const char* fmt, const char* arg1) {
    const char* args[] = {arg1};
    return nova_util_format(fmt, args, 1);
}

// Format with two arguments
const char* nova_util_format2(const char* fmt, const char* arg1, const char* arg2) {
    const char* args[] = {arg1, arg2};
    return nova_util_format(fmt, args, 2);
}

// Format with three arguments
const char* nova_util_format3(const char* fmt, const char* arg1, const char* arg2, const char* arg3) {
    const char* args[] = {arg1, arg2, arg3};
    return nova_util_format(fmt, args, 3);
}

// ============================================================================
// util.inspect(object, options)
// ============================================================================

const char* nova_util_inspect(const char* value, int depth, int colors, int showHidden) {
    if (!value) return "undefined";

    static thread_local std::string result;
    result.clear();

    // Simple implementation - just return the value for primitives
    // For objects, we'd need type information
    (void)depth;
    (void)colors;
    (void)showHidden;

    result = value;
    return result.c_str();
}

// Inspect with default options
const char* nova_util_inspect_default(const char* value) {
    return nova_util_inspect(value, 2, 0, 0);
}

// ============================================================================
// util.isDeepStrictEqual(val1, val2)
// ============================================================================

int nova_util_isDeepStrictEqual(const char* val1, const char* val2) {
    if (val1 == val2) return 1;
    if (!val1 || !val2) return 0;
    return strcmp(val1, val2) == 0 ? 1 : 0;
}

// ============================================================================
// util.deprecate(fn, msg, code)
// ============================================================================

// Store deprecation warnings that have been shown
static std::set<std::string>* shownDeprecations = nullptr;

void nova_util_deprecate_warn(const char* msg, const char* code) {
    if (!shownDeprecations) {
        shownDeprecations = new std::set<std::string>();
    }

    std::string key = code ? code : msg;
    if (shownDeprecations->find(key) != shownDeprecations->end()) {
        return;  // Already shown
    }
    shownDeprecations->insert(key);

    fprintf(stderr, "(node:util) [%s] DeprecationWarning: %s\n",
            code ? code : "DEP0000", msg ? msg : "");
}

// ============================================================================
// util.debuglog(section)
// ============================================================================

static std::set<std::string>* enabledSections = nullptr;
static bool debuglogInitialized = false;

static void initDebuglog() {
    if (debuglogInitialized) return;
    debuglogInitialized = true;

    enabledSections = new std::set<std::string>();

    const char* nodeDebug = getenv("NODE_DEBUG");
    if (nodeDebug) {
        std::string sections = nodeDebug;
        std::transform(sections.begin(), sections.end(), sections.begin(), ::toupper);

        size_t pos = 0;
        size_t found;
        while ((found = sections.find(',', pos)) != std::string::npos) {
            std::string sec = sections.substr(pos, found - pos);
            // Trim whitespace
            size_t start = sec.find_first_not_of(" \t");
            size_t end = sec.find_last_not_of(" \t");
            if (start != std::string::npos) {
                enabledSections->insert(sec.substr(start, end - start + 1));
            }
            pos = found + 1;
        }
        std::string sec = sections.substr(pos);
        size_t start = sec.find_first_not_of(" \t");
        size_t end = sec.find_last_not_of(" \t");
        if (start != std::string::npos) {
            enabledSections->insert(sec.substr(start, end - start + 1));
        }
    }
}

int nova_util_debuglog_enabled(const char* section) {
    initDebuglog();
    if (!section || !enabledSections) return 0;

    std::string sec = section;
    std::transform(sec.begin(), sec.end(), sec.begin(), ::toupper);
    return enabledSections->find(sec) != enabledSections->end() ? 1 : 0;
}

void nova_util_debuglog(const char* section, const char* msg) {
    if (!nova_util_debuglog_enabled(section)) return;

    std::string sec = section;
    std::transform(sec.begin(), sec.end(), sec.begin(), ::toupper);
#ifdef _WIN32
    fprintf(stderr, "%s %lu: %s\n", sec.c_str(),
            (unsigned long)GetCurrentProcessId(),
            msg ? msg : "");
#else
    fprintf(stderr, "%s %d: %s\n", sec.c_str(),
            getpid(),
            msg ? msg : "");
#endif
}

// ============================================================================
// util.getSystemErrorName(err)
// ============================================================================

const char* nova_util_getSystemErrorName(int err) {
    static thread_local char buffer[32];

#ifdef _WIN32
    // Windows error codes
    switch (err) {
        case 0: return "OK";
        case 1: return "EPERM";
        case 2: return "ENOENT";
        case 3: return "ESRCH";
        case 4: return "EINTR";
        case 5: return "EIO";
        case 6: return "ENXIO";
        case 7: return "E2BIG";
        case 8: return "ENOEXEC";
        case 9: return "EBADF";
        case 10: return "ECHILD";
        case 11: return "EAGAIN";
        case 12: return "ENOMEM";
        case 13: return "EACCES";
        case 14: return "EFAULT";
        case 16: return "EBUSY";
        case 17: return "EEXIST";
        case 18: return "EXDEV";
        case 19: return "ENODEV";
        case 20: return "ENOTDIR";
        case 21: return "EISDIR";
        case 22: return "EINVAL";
        case 23: return "ENFILE";
        case 24: return "EMFILE";
        case 25: return "ENOTTY";
        case 27: return "EFBIG";
        case 28: return "ENOSPC";
        case 29: return "ESPIPE";
        case 30: return "EROFS";
        case 31: return "EMLINK";
        case 32: return "EPIPE";
        case 33: return "EDOM";
        case 34: return "ERANGE";
        case 36: return "EDEADLK";
        case 38: return "ENAMETOOLONG";
        case 39: return "ENOLCK";
        case 40: return "ENOSYS";
        case 41: return "ENOTEMPTY";
        default:
            snprintf(buffer, sizeof(buffer), "Unknown system error %d", err);
            return buffer;
    }
#else
    // POSIX error codes
    switch (err) {
        case 0: return "OK";
        case EPERM: return "EPERM";
        case ENOENT: return "ENOENT";
        case ESRCH: return "ESRCH";
        case EINTR: return "EINTR";
        case EIO: return "EIO";
        case ENXIO: return "ENXIO";
        case E2BIG: return "E2BIG";
        case ENOEXEC: return "ENOEXEC";
        case EBADF: return "EBADF";
        case ECHILD: return "ECHILD";
        case EAGAIN: return "EAGAIN";
        case ENOMEM: return "ENOMEM";
        case EACCES: return "EACCES";
        case EFAULT: return "EFAULT";
        case EBUSY: return "EBUSY";
        case EEXIST: return "EEXIST";
        case EXDEV: return "EXDEV";
        case ENODEV: return "ENODEV";
        case ENOTDIR: return "ENOTDIR";
        case EISDIR: return "EISDIR";
        case EINVAL: return "EINVAL";
        case ENFILE: return "ENFILE";
        case EMFILE: return "EMFILE";
        case ENOTTY: return "ENOTTY";
        case EFBIG: return "EFBIG";
        case ENOSPC: return "ENOSPC";
        case ESPIPE: return "ESPIPE";
        case EROFS: return "EROFS";
        case EMLINK: return "EMLINK";
        case EPIPE: return "EPIPE";
        case EDOM: return "EDOM";
        case ERANGE: return "ERANGE";
        case EDEADLK: return "EDEADLK";
        case ENAMETOOLONG: return "ENAMETOOLONG";
        case ENOLCK: return "ENOLCK";
        case ENOSYS: return "ENOSYS";
        case ENOTEMPTY: return "ENOTEMPTY";
        default:
            snprintf(buffer, sizeof(buffer), "Unknown system error %d", err);
            return buffer;
    }
#endif
}

// ============================================================================
// util.stripVTControlCharacters(str)
// ============================================================================

const char* nova_util_stripVTControlCharacters(const char* str) {
    if (!str) return "";

    static thread_local std::string result;
    result.clear();

    const char* p = str;
    while (*p) {
        // Skip ANSI escape sequences
        if (*p == '\x1b' && *(p + 1) == '[') {
            p += 2;
            // Skip until we find the end of the sequence
            while (*p && !isalpha(*p)) p++;
            if (*p) p++;  // Skip the final letter
            continue;
        }
        // Skip other control characters
        if (*p == '\x1b' || (*p >= 0 && *p < 32 && *p != '\n' && *p != '\r' && *p != '\t')) {
            p++;
            continue;
        }
        result += *p++;
    }

    return result.c_str();
}

// ============================================================================
// util.toUSVString(string)
// ============================================================================

const char* nova_util_toUSVString(const char* str) {
    if (!str) return "";
    // Simple implementation - return as-is for ASCII
    // Full implementation would handle surrogate pairs
    static thread_local std::string result;
    result = str;
    return result.c_str();
}

// ============================================================================
// util.styleText(format, text)
// ============================================================================

const char* nova_util_styleText(const char* format, const char* text) {
    if (!text) return "";

    static thread_local std::string result;

    // ANSI color codes
    const char* code = "";
    const char* reset = "\x1b[0m";

    if (format) {
        if (strcmp(format, "red") == 0) code = "\x1b[31m";
        else if (strcmp(format, "green") == 0) code = "\x1b[32m";
        else if (strcmp(format, "yellow") == 0) code = "\x1b[33m";
        else if (strcmp(format, "blue") == 0) code = "\x1b[34m";
        else if (strcmp(format, "magenta") == 0) code = "\x1b[35m";
        else if (strcmp(format, "cyan") == 0) code = "\x1b[36m";
        else if (strcmp(format, "white") == 0) code = "\x1b[37m";
        else if (strcmp(format, "black") == 0) code = "\x1b[30m";
        else if (strcmp(format, "bold") == 0) code = "\x1b[1m";
        else if (strcmp(format, "dim") == 0) code = "\x1b[2m";
        else if (strcmp(format, "italic") == 0) code = "\x1b[3m";
        else if (strcmp(format, "underline") == 0) code = "\x1b[4m";
        else if (strcmp(format, "inverse") == 0) code = "\x1b[7m";
        else if (strcmp(format, "strikethrough") == 0) code = "\x1b[9m";
        else if (strcmp(format, "bgRed") == 0) code = "\x1b[41m";
        else if (strcmp(format, "bgGreen") == 0) code = "\x1b[42m";
        else if (strcmp(format, "bgYellow") == 0) code = "\x1b[43m";
        else if (strcmp(format, "bgBlue") == 0) code = "\x1b[44m";
        else if (strcmp(format, "bgMagenta") == 0) code = "\x1b[45m";
        else if (strcmp(format, "bgCyan") == 0) code = "\x1b[46m";
        else if (strcmp(format, "bgWhite") == 0) code = "\x1b[47m";
    }

    result = std::string(code) + text + reset;
    return result.c_str();
}

// ============================================================================
// util.types.* - Type checking functions
// ============================================================================

int nova_util_types_isArrayBuffer(const char* typeTag) {
    return typeTag && strcmp(typeTag, "ArrayBuffer") == 0;
}

int nova_util_types_isSharedArrayBuffer(const char* typeTag) {
    return typeTag && strcmp(typeTag, "SharedArrayBuffer") == 0;
}

int nova_util_types_isDataView(const char* typeTag) {
    return typeTag && strcmp(typeTag, "DataView") == 0;
}

int nova_util_types_isDate(const char* typeTag) {
    return typeTag && strcmp(typeTag, "Date") == 0;
}

int nova_util_types_isMap(const char* typeTag) {
    return typeTag && strcmp(typeTag, "Map") == 0;
}

int nova_util_types_isSet(const char* typeTag) {
    return typeTag && strcmp(typeTag, "Set") == 0;
}

int nova_util_types_isWeakMap(const char* typeTag) {
    return typeTag && strcmp(typeTag, "WeakMap") == 0;
}

int nova_util_types_isWeakSet(const char* typeTag) {
    return typeTag && strcmp(typeTag, "WeakSet") == 0;
}

int nova_util_types_isRegExp(const char* typeTag) {
    return typeTag && strcmp(typeTag, "RegExp") == 0;
}

int nova_util_types_isPromise(const char* typeTag) {
    return typeTag && strcmp(typeTag, "Promise") == 0;
}

int nova_util_types_isGeneratorFunction(const char* typeTag) {
    return typeTag && strcmp(typeTag, "GeneratorFunction") == 0;
}

int nova_util_types_isGeneratorObject(const char* typeTag) {
    return typeTag && strcmp(typeTag, "Generator") == 0;
}

int nova_util_types_isAsyncFunction(const char* typeTag) {
    return typeTag && strcmp(typeTag, "AsyncFunction") == 0;
}

int nova_util_types_isAsyncGeneratorFunction(const char* typeTag) {
    return typeTag && strcmp(typeTag, "AsyncGeneratorFunction") == 0;
}

int nova_util_types_isAsyncGeneratorObject(const char* typeTag) {
    return typeTag && strcmp(typeTag, "AsyncGenerator") == 0;
}

int nova_util_types_isMapIterator(const char* typeTag) {
    return typeTag && strcmp(typeTag, "MapIterator") == 0;
}

int nova_util_types_isSetIterator(const char* typeTag) {
    return typeTag && strcmp(typeTag, "SetIterator") == 0;
}

int nova_util_types_isStringObject(const char* typeTag) {
    return typeTag && strcmp(typeTag, "String") == 0;
}

int nova_util_types_isNumberObject(const char* typeTag) {
    return typeTag && strcmp(typeTag, "Number") == 0;
}

int nova_util_types_isBooleanObject(const char* typeTag) {
    return typeTag && strcmp(typeTag, "Boolean") == 0;
}

int nova_util_types_isBigIntObject(const char* typeTag) {
    return typeTag && strcmp(typeTag, "BigInt") == 0;
}

int nova_util_types_isSymbolObject(const char* typeTag) {
    return typeTag && strcmp(typeTag, "Symbol") == 0;
}

int nova_util_types_isNativeError(const char* typeTag) {
    if (!typeTag) return 0;
    return strcmp(typeTag, "Error") == 0 ||
           strcmp(typeTag, "TypeError") == 0 ||
           strcmp(typeTag, "RangeError") == 0 ||
           strcmp(typeTag, "SyntaxError") == 0 ||
           strcmp(typeTag, "ReferenceError") == 0 ||
           strcmp(typeTag, "EvalError") == 0 ||
           strcmp(typeTag, "URIError") == 0;
}

int nova_util_types_isBoxedPrimitive(const char* typeTag) {
    return nova_util_types_isStringObject(typeTag) ||
           nova_util_types_isNumberObject(typeTag) ||
           nova_util_types_isBooleanObject(typeTag) ||
           nova_util_types_isBigIntObject(typeTag) ||
           nova_util_types_isSymbolObject(typeTag);
}

int nova_util_types_isTypedArray(const char* typeTag) {
    if (!typeTag) return 0;
    return strcmp(typeTag, "Int8Array") == 0 ||
           strcmp(typeTag, "Uint8Array") == 0 ||
           strcmp(typeTag, "Uint8ClampedArray") == 0 ||
           strcmp(typeTag, "Int16Array") == 0 ||
           strcmp(typeTag, "Uint16Array") == 0 ||
           strcmp(typeTag, "Int32Array") == 0 ||
           strcmp(typeTag, "Uint32Array") == 0 ||
           strcmp(typeTag, "Float32Array") == 0 ||
           strcmp(typeTag, "Float64Array") == 0 ||
           strcmp(typeTag, "BigInt64Array") == 0 ||
           strcmp(typeTag, "BigUint64Array") == 0;
}

int nova_util_types_isInt8Array(const char* typeTag) {
    return typeTag && strcmp(typeTag, "Int8Array") == 0;
}

int nova_util_types_isUint8Array(const char* typeTag) {
    return typeTag && strcmp(typeTag, "Uint8Array") == 0;
}

int nova_util_types_isUint8ClampedArray(const char* typeTag) {
    return typeTag && strcmp(typeTag, "Uint8ClampedArray") == 0;
}

int nova_util_types_isInt16Array(const char* typeTag) {
    return typeTag && strcmp(typeTag, "Int16Array") == 0;
}

int nova_util_types_isUint16Array(const char* typeTag) {
    return typeTag && strcmp(typeTag, "Uint16Array") == 0;
}

int nova_util_types_isInt32Array(const char* typeTag) {
    return typeTag && strcmp(typeTag, "Int32Array") == 0;
}

int nova_util_types_isUint32Array(const char* typeTag) {
    return typeTag && strcmp(typeTag, "Uint32Array") == 0;
}

int nova_util_types_isFloat32Array(const char* typeTag) {
    return typeTag && strcmp(typeTag, "Float32Array") == 0;
}

int nova_util_types_isFloat64Array(const char* typeTag) {
    return typeTag && strcmp(typeTag, "Float64Array") == 0;
}

int nova_util_types_isBigInt64Array(const char* typeTag) {
    return typeTag && strcmp(typeTag, "BigInt64Array") == 0;
}

int nova_util_types_isBigUint64Array(const char* typeTag) {
    return typeTag && strcmp(typeTag, "BigUint64Array") == 0;
}

int nova_util_types_isAnyArrayBuffer(const char* typeTag) {
    return nova_util_types_isArrayBuffer(typeTag) ||
           nova_util_types_isSharedArrayBuffer(typeTag);
}

int nova_util_types_isArrayBufferView(const char* typeTag) {
    return nova_util_types_isTypedArray(typeTag) ||
           nova_util_types_isDataView(typeTag);
}

int nova_util_types_isExternal(const char* typeTag) {
    return typeTag && strcmp(typeTag, "External") == 0;
}

int nova_util_types_isProxy(const char* typeTag) {
    return typeTag && strcmp(typeTag, "Proxy") == 0;
}

int nova_util_types_isModuleNamespaceObject(const char* typeTag) {
    return typeTag && strcmp(typeTag, "Module") == 0;
}

int nova_util_types_isArgumentsObject(const char* typeTag) {
    return typeTag && strcmp(typeTag, "Arguments") == 0;
}

// ============================================================================
// Legacy type checking (deprecated but still used)
// ============================================================================

int nova_util_isArray(const char* typeTag) {
    return typeTag && strcmp(typeTag, "Array") == 0;
}

int nova_util_isBoolean(const char* value) {
    return value && (strcmp(value, "true") == 0 || strcmp(value, "false") == 0);
}

int nova_util_isNull(const char* value) {
    return value && strcmp(value, "null") == 0;
}

int nova_util_isNullOrUndefined(const char* value) {
    return !value || strcmp(value, "null") == 0 || strcmp(value, "undefined") == 0;
}

int nova_util_isNumber(const char* typeTag) {
    return typeTag && strcmp(typeTag, "number") == 0;
}

int nova_util_isString(const char* typeTag) {
    return typeTag && strcmp(typeTag, "string") == 0;
}

int nova_util_isSymbol(const char* typeTag) {
    return typeTag && strcmp(typeTag, "symbol") == 0;
}

int nova_util_isUndefined(const char* value) {
    return !value || strcmp(value, "undefined") == 0;
}

int nova_util_isRegExp(const char* typeTag) {
    return nova_util_types_isRegExp(typeTag);
}

int nova_util_isObject(const char* typeTag) {
    return typeTag && strcmp(typeTag, "object") == 0;
}

int nova_util_isDate(const char* typeTag) {
    return nova_util_types_isDate(typeTag);
}

int nova_util_isError(const char* typeTag) {
    return nova_util_types_isNativeError(typeTag);
}

int nova_util_isFunction(const char* typeTag) {
    return typeTag && strcmp(typeTag, "function") == 0;
}

int nova_util_isPrimitive(const char* typeTag) {
    if (!typeTag) return 1;  // undefined is primitive
    return strcmp(typeTag, "string") == 0 ||
           strcmp(typeTag, "number") == 0 ||
           strcmp(typeTag, "boolean") == 0 ||
           strcmp(typeTag, "symbol") == 0 ||
           strcmp(typeTag, "bigint") == 0 ||
           strcmp(typeTag, "undefined") == 0 ||
           strcmp(typeTag, "null") == 0;
}

int nova_util_isBuffer(const char* typeTag) {
    return typeTag && strcmp(typeTag, "Buffer") == 0;
}

// ============================================================================
// util.parseArgs - Command line argument parsing
// ============================================================================

struct ParsedArg {
    std::string name;
    std::string value;
    bool isPositional;
};

// Helper: Simple JSON parser for parseArgs options
static bool parseArgsGetBool(const std::string& json, const std::string& key, bool defaultVal) {
    std::string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return defaultVal;
    pos += search.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    if (pos < json.length()) {
        if (json.substr(pos, 4) == "true") return true;
        if (json.substr(pos, 5) == "false") return false;
    }
    return defaultVal;
}

static bool parseArgsIsTypeString(const std::string& json, const std::string& name) {
    std::string search = "\"" + name + "\":{";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return false;
    size_t typePos = json.find("\"type\":\"string\"", pos);
    size_t endPos = json.find("}", pos);
    return typePos != std::string::npos && typePos < endPos;
}

const char* nova_util_parseArgs(const char** args, int argCount, const char* optionsJson) {
    static thread_local std::string result;
    result = "{\"values\":{},\"positionals\":[]}";

    if (!args || argCount == 0) return result.c_str();

    // Parse options
    std::string opts = optionsJson ? optionsJson : "{}";
    bool strict = parseArgsGetBool(opts, "strict", false);
    bool allowPositionals = parseArgsGetBool(opts, "allowPositionals", true);

    std::ostringstream values;
    std::ostringstream positionals;
    std::ostringstream tokens;
    values << "{";
    positionals << "[";
    tokens << "[";

    bool firstValue = true;
    bool firstPositional = true;
    bool firstToken = true;

    for (int i = 0; i < argCount; i++) {
        if (!args[i]) continue;

        std::string arg = args[i];

        // Add to tokens
        if (!firstToken) tokens << ",";
        tokens << "{\"kind\":";

        if (arg.length() > 2 && arg[0] == '-' && arg[1] == '-') {
            // Long option: --name or --name=value
            std::string name = arg.substr(2);
            std::string value = "true";
            bool isTypeString = parseArgsIsTypeString(opts, name);

            size_t eqPos = name.find('=');
            if (eqPos != std::string::npos) {
                value = name.substr(eqPos + 1);
                name = name.substr(0, eqPos);
            } else if (isTypeString && i + 1 < argCount && args[i + 1][0] != '-') {
                value = args[++i];
            }

            if (!firstValue) values << ",";
            if (isTypeString || value != "true") {
                values << "\"" << name << "\":\"" << value << "\"";
            } else {
                values << "\"" << name << "\":true";
            }
            firstValue = false;

            tokens << "\"option\",\"name\":\"" << name << "\",\"value\":";
            if (isTypeString || value != "true") {
                tokens << "\"" << value << "\"";
            } else {
                tokens << "true";
            }
            tokens << "}";
        } else if (arg.length() > 1 && arg[0] == '-') {
            // Short option: -n or -n value
            std::string shortName = arg.substr(1);
            std::string value = "true";

            // Try to find long name for this short option
            std::string longName = shortName;
            bool isTypeString = parseArgsIsTypeString(opts, longName);

            if (isTypeString && i + 1 < argCount && args[i + 1][0] != '-') {
                value = args[++i];
            }

            if (!firstValue) values << ",";
            if (isTypeString || value != "true") {
                values << "\"" << longName << "\":\"" << value << "\"";
            } else {
                values << "\"" << longName << "\":true";
            }
            firstValue = false;

            tokens << "\"option\",\"name\":\"" << longName << "\",\"value\":";
            if (isTypeString || value != "true") {
                tokens << "\"" << value << "\"";
            } else {
                tokens << "true";
            }
            tokens << "}";
        } else {
            // Positional argument
            if (allowPositionals || !strict) {
                if (!firstPositional) positionals << ",";
                positionals << "\"" << arg << "\"";
                firstPositional = false;
            }
            tokens << "\"positional\",\"value\":\"" << arg << "\"}";
        }
        firstToken = false;
    }

    values << "}";
    positionals << "]";
    tokens << "]";

    result = "{\"values\":" + values.str() + ",\"positionals\":" + positionals.str() +
             ",\"tokens\":" + tokens.str() + "}";
    return result.c_str();
}

// ============================================================================
// util.parseEnv(content)
// ============================================================================

const char* nova_util_parseEnv(const char* content) {
    if (!content) return "{}";

    static thread_local std::string result;
    std::ostringstream json;
    json << "{";

    std::istringstream stream(content);
    std::string line;
    bool first = true;

    while (std::getline(stream, line)) {
        // Skip empty lines and comments
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos || line[start] == '#') continue;

        // Find = sign
        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = line.substr(0, eqPos);
        std::string value = line.substr(eqPos + 1);

        // Trim key
        size_t keyStart = key.find_first_not_of(" \t");
        size_t keyEnd = key.find_last_not_of(" \t");
        if (keyStart != std::string::npos) {
            key = key.substr(keyStart, keyEnd - keyStart + 1);
        }

        // Handle quoted values
        size_t valStart = value.find_first_not_of(" \t");
        if (valStart != std::string::npos) {
            value = value.substr(valStart);
            if ((value[0] == '"' && value.back() == '"') ||
                (value[0] == '\'' && value.back() == '\'')) {
                value = value.substr(1, value.length() - 2);
            }
        }

        if (!first) json << ",";
        json << "\"" << key << "\":\"" << value << "\"";
        first = false;
    }

    json << "}";
    result = json.str();
    return result.c_str();
}

// ============================================================================
// util.TextEncoder / util.TextDecoder (basic implementation)
// ============================================================================

const char* nova_util_TextEncoder_encode(const char* str) {
    // Returns comma-separated byte values
    if (!str) return "";

    static thread_local std::string result;
    result.clear();

    bool first = true;
    for (const unsigned char* p = (const unsigned char*)str; *p; p++) {
        if (!first) result += ",";
        result += std::to_string(*p);
        first = false;
    }

    return result.c_str();
}

const char* nova_util_TextDecoder_decode(const unsigned char* bytes, int length) {
    if (!bytes || length <= 0) return "";

    static thread_local std::string result;
    result.assign((const char*)bytes, length);
    return result.c_str();
}

// ============================================================================
// util.MIMEType (basic implementation)
// ============================================================================

const char* nova_util_MIMEType_parse(const char* input) {
    if (!input) return "{}";

    static thread_local std::string result;
    std::string str = input;

    // Parse: type/subtype;param=value
    std::string type, subtype;
    std::ostringstream params;
    params << "{";

    size_t slashPos = str.find('/');
    if (slashPos == std::string::npos) {
        result = "{}";
        return result.c_str();
    }

    type = str.substr(0, slashPos);
    str = str.substr(slashPos + 1);

    size_t semicolonPos = str.find(';');
    if (semicolonPos != std::string::npos) {
        subtype = str.substr(0, semicolonPos);
        str = str.substr(semicolonPos + 1);

        // Parse parameters
        bool firstParam = true;
        while (!str.empty()) {
            size_t nextSemi = str.find(';');
            std::string param = (nextSemi != std::string::npos) ? str.substr(0, nextSemi) : str;

            size_t eqPos = param.find('=');
            if (eqPos != std::string::npos) {
                std::string key = param.substr(0, eqPos);
                std::string val = param.substr(eqPos + 1);
                // Trim
                size_t ks = key.find_first_not_of(" \t");
                size_t ke = key.find_last_not_of(" \t");
                if (ks != std::string::npos) key = key.substr(ks, ke - ks + 1);

                if (!firstParam) params << ",";
                params << "\"" << key << "\":\"" << val << "\"";
                firstParam = false;
            }

            if (nextSemi == std::string::npos) break;
            str = str.substr(nextSemi + 1);
        }
    } else {
        subtype = str;
    }

    params << "}";

    std::ostringstream json;
    json << "{\"type\":\"" << type << "\",\"subtype\":\"" << subtype
         << "\",\"essence\":\"" << type << "/" << subtype
         << "\",\"params\":" << params.str() << "}";

    result = json.str();
    return result.c_str();
}

// ============================================================================
// Cleanup
// ============================================================================

void nova_util_cleanup() {
    if (shownDeprecations) {
        delete shownDeprecations;
        shownDeprecations = nullptr;
    }
    if (enabledSections) {
        delete enabledSections;
        enabledSections = nullptr;
    }
    debuglogInitialized = false;
}

} // extern "C"
