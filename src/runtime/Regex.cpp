// Nova Runtime - Regular Expression Support
// Uses C++ <regex> for pattern matching

#include <regex>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <vector>

// Forward declarations for array creation
namespace nova { namespace runtime {
    struct ValueArray {
        struct { size_t size; uint32_t type_id; bool is_marked; void* next; } header;
        int64_t length;
        int64_t capacity;
        int64_t* elements;
    };
    ValueArray* create_value_array(int64_t capacity);
    void* create_metadata_from_value_array(ValueArray* array);
}}

extern "C" {

// Regex object structure
struct NovaRegex {
    char* pattern;
    char* flags;
    std::regex* compiled;
    std::regex_constants::syntax_option_type options;
    bool global;
    bool ignoreCase;
    bool multiline;
    bool dotAll;
    bool unicode;
    bool sticky;
    bool hasIndices;    // ES2022 'd' flag
    bool unicodeSets;   // ES2024 'v' flag
    int64_t lastIndex;
};

// Create a regex object from pattern and flags
void* nova_regex_create(const char* pattern, const char* flags) {
    if (!pattern) return nullptr;

    NovaRegex* regex = new NovaRegex();
    regex->pattern = strdup(pattern);
    regex->flags = flags ? strdup(flags) : strdup("");
    regex->lastIndex = 0;

    // Parse flags
    regex->global = false;
    regex->ignoreCase = false;
    regex->multiline = false;
    regex->dotAll = false;
    regex->unicode = false;
    regex->sticky = false;
    regex->hasIndices = false;
    regex->unicodeSets = false;

    std::regex_constants::syntax_option_type options = std::regex_constants::ECMAScript;

    if (flags) {
        for (const char* p = flags; *p; ++p) {
            switch (*p) {
                case 'g': regex->global = true; break;
                case 'i':
                    regex->ignoreCase = true;
                    options |= std::regex_constants::icase;
                    break;
                case 'm': regex->multiline = true; break;
                case 's': regex->dotAll = true; break;
                case 'u': regex->unicode = true; break;
                case 'y': regex->sticky = true; break;
                case 'd': regex->hasIndices = true; break;  // ES2022
                case 'v': regex->unicodeSets = true; break; // ES2024
            }
        }
    }

    regex->options = options;

    try {
        regex->compiled = new std::regex(pattern, options);
    } catch (const std::regex_error& e) {
        std::cerr << "Regex error: " << e.what() << std::endl;
        regex->compiled = nullptr;
    }

    return regex;
}

// Free a regex object
void nova_regex_free(void* regexPtr) {
    if (!regexPtr) return;
    NovaRegex* regex = static_cast<NovaRegex*>(regexPtr);
    if (regex->pattern) free(regex->pattern);
    if (regex->flags) free(regex->flags);
    if (regex->compiled) delete regex->compiled;
    delete regex;
}

// Test if a string matches the regex
int64_t nova_regex_test(void* regexPtr, const char* str) {
    if (!regexPtr || !str) return 0;
    NovaRegex* regex = static_cast<NovaRegex*>(regexPtr);
    if (!regex->compiled) return 0;

    try {
        std::string s(str);
        if (regex->sticky) {
            // Sticky mode: match must start at lastIndex
            if (regex->lastIndex >= (int64_t)s.length()) {
                regex->lastIndex = 0;
                return 0;
            }
            std::smatch match;
            std::string sub = s.substr(regex->lastIndex);
            if (std::regex_search(sub, match, *regex->compiled) && match.position() == 0) {
                regex->lastIndex += match.length();
                return 1;
            }
            regex->lastIndex = 0;
            return 0;
        } else {
            return std::regex_search(s, *regex->compiled) ? 1 : 0;
        }
    } catch (...) {
        return 0;
    }
}

// Execute regex and return match array (simplified: returns first match as string)
const char* nova_regex_exec(void* regexPtr, const char* str) {
    if (!regexPtr || !str) return nullptr;
    NovaRegex* regex = static_cast<NovaRegex*>(regexPtr);
    if (!regex->compiled) return nullptr;

    try {
        std::string s(str);
        std::smatch match;

        if (regex->sticky) {
            if (regex->lastIndex >= (int64_t)s.length()) {
                regex->lastIndex = 0;
                return nullptr;
            }
            std::string sub = s.substr(regex->lastIndex);
            if (std::regex_search(sub, match, *regex->compiled) && match.position() == 0) {
                regex->lastIndex += match.position() + match.length();
                return strdup(match.str().c_str());
            }
            regex->lastIndex = 0;
            return nullptr;
        } else if (regex->global) {
            if (regex->lastIndex >= (int64_t)s.length()) {
                regex->lastIndex = 0;
                return nullptr;
            }
            std::string sub = s.substr(regex->lastIndex);
            if (std::regex_search(sub, match, *regex->compiled)) {
                regex->lastIndex += match.position() + match.length();
                return strdup(match.str().c_str());
            }
            regex->lastIndex = 0;
            return nullptr;
        } else {
            if (std::regex_search(s, match, *regex->compiled)) {
                return strdup(match.str().c_str());
            }
            return nullptr;
        }
    } catch (...) {
        return nullptr;
    }
}

// String.match(regex) - returns matched string or null
const char* nova_string_match(const char* str, void* regexPtr) {
    if (!str || !regexPtr) return nullptr;
    NovaRegex* regex = static_cast<NovaRegex*>(regexPtr);
    if (!regex->compiled) return nullptr;

    try {
        std::string s(str);
        std::smatch match;

        if (std::regex_search(s, match, *regex->compiled)) {
            return strdup(match.str().c_str());
        }
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

// String.replace(regex, replacement) - replace first/all matches
const char* nova_string_replace_regex(const char* str, void* regexPtr, const char* replacement) {
    if (!str || !regexPtr || !replacement) return strdup(str ? str : "");
    NovaRegex* regex = static_cast<NovaRegex*>(regexPtr);
    if (!regex->compiled) return strdup(str);

    try {
        std::string s(str);
        std::string repl(replacement);
        std::string result;

        if (regex->global) {
            result = std::regex_replace(s, *regex->compiled, repl);
        } else {
            // Replace only first match
            result = std::regex_replace(s, *regex->compiled, repl,
                std::regex_constants::format_first_only);
        }

        return strdup(result.c_str());
    } catch (...) {
        return strdup(str);
    }
}

// String.search(regex) - returns index of first match or -1
int64_t nova_string_search(const char* str, void* regexPtr) {
    if (!str || !regexPtr) return -1;
    NovaRegex* regex = static_cast<NovaRegex*>(regexPtr);
    if (!regex->compiled) return -1;

    try {
        std::string s(str);
        std::smatch match;

        if (std::regex_search(s, match, *regex->compiled)) {
            return match.position();
        }
        return -1;
    } catch (...) {
        return -1;
    }
}

// String.split(regex) - split string by regex pattern
// Returns a pointer to array metadata (nova value array format)
void* nova_string_split_regex(const char* str, void* regexPtr) {
    if (!str) {
        // Return empty array
        return nullptr;
    }

    NovaRegex* regex = regexPtr ? static_cast<NovaRegex*>(regexPtr) : nullptr;
    std::string s(str);
    std::vector<std::string> parts;

    if (!regex || !regex->compiled) {
        // If no valid regex, return array with original string
        parts.push_back(s);
    } else {
        try {
            std::sregex_token_iterator iter(s.begin(), s.end(), *regex->compiled, -1);
            std::sregex_token_iterator end;
            for (; iter != end; ++iter) {
                parts.push_back(*iter);
            }
        } catch (...) {
            parts.push_back(s);
        }
    }

    // Create proper value array with string elements
    int64_t count = static_cast<int64_t>(parts.size());
    nova::runtime::ValueArray* resultArray = nova::runtime::create_value_array(count);
    resultArray->length = count;

    for (int64_t i = 0; i < count; i++) {
        char* copy = strdup(parts[i].c_str());
        resultArray->elements[i] = reinterpret_cast<int64_t>(copy);
    }

    return nova::runtime::create_metadata_from_value_array(resultArray);
}

// Get regex pattern
const char* nova_regex_get_pattern(void* regexPtr) {
    if (!regexPtr) return "";
    NovaRegex* regex = static_cast<NovaRegex*>(regexPtr);
    return regex->pattern ? regex->pattern : "";
}

// Get regex flags
const char* nova_regex_get_flags(void* regexPtr) {
    if (!regexPtr) return "";
    NovaRegex* regex = static_cast<NovaRegex*>(regexPtr);
    return regex->flags ? regex->flags : "";
}

// Get/set lastIndex property
int64_t nova_regex_get_lastIndex(void* regexPtr) {
    if (!regexPtr) return 0;
    NovaRegex* regex = static_cast<NovaRegex*>(regexPtr);
    return regex->lastIndex;
}

void nova_regex_set_lastIndex(void* regexPtr, int64_t index) {
    if (!regexPtr) return;
    NovaRegex* regex = static_cast<NovaRegex*>(regexPtr);
    regex->lastIndex = index;
}

// Check regex flags
int64_t nova_regex_get_global(void* regexPtr) {
    if (!regexPtr) return 0;
    return static_cast<NovaRegex*>(regexPtr)->global ? 1 : 0;
}

int64_t nova_regex_get_ignoreCase(void* regexPtr) {
    if (!regexPtr) return 0;
    return static_cast<NovaRegex*>(regexPtr)->ignoreCase ? 1 : 0;
}

int64_t nova_regex_get_multiline(void* regexPtr) {
    if (!regexPtr) return 0;
    return static_cast<NovaRegex*>(regexPtr)->multiline ? 1 : 0;
}

int64_t nova_regex_get_dotAll(void* regexPtr) {
    if (!regexPtr) return 0;
    return static_cast<NovaRegex*>(regexPtr)->dotAll ? 1 : 0;
}

int64_t nova_regex_get_unicode(void* regexPtr) {
    if (!regexPtr) return 0;
    return static_cast<NovaRegex*>(regexPtr)->unicode ? 1 : 0;
}

int64_t nova_regex_get_sticky(void* regexPtr) {
    if (!regexPtr) return 0;
    return static_cast<NovaRegex*>(regexPtr)->sticky ? 1 : 0;
}

// ES2022: hasIndices property (d flag)
int64_t nova_regex_get_hasIndices(void* regexPtr) {
    if (!regexPtr) return 0;
    return static_cast<NovaRegex*>(regexPtr)->hasIndices ? 1 : 0;
}

// ES2024: unicodeSets property (v flag)
int64_t nova_regex_get_unicodeSets(void* regexPtr) {
    if (!regexPtr) return 0;
    return static_cast<NovaRegex*>(regexPtr)->unicodeSets ? 1 : 0;
}

// toString() - returns "/pattern/flags"
const char* nova_regex_toString(void* regexPtr) {
    if (!regexPtr) return "/(?:)/";
    NovaRegex* regex = static_cast<NovaRegex*>(regexPtr);

    // Build the string: /pattern/flags
    std::string result = "/";
    result += regex->pattern ? regex->pattern : "";
    result += "/";
    result += regex->flags ? regex->flags : "";

    return strdup(result.c_str());
}

// matchAll() - returns iterator of all matches (ES2020)
// For simplicity, returns array of match strings
void* nova_regex_matchAll(void* regexPtr, const char* str) {
    if (!regexPtr || !str) return nullptr;
    NovaRegex* regex = static_cast<NovaRegex*>(regexPtr);
    if (!regex->compiled) return nullptr;

    // matchAll requires global flag
    if (!regex->global) {
        std::cerr << "TypeError: matchAll must be called with a global RegExp" << std::endl;
        return nullptr;
    }

    try {
        std::string s(str);
        std::vector<std::string> matches;

        std::sregex_iterator iter(s.begin(), s.end(), *regex->compiled);
        std::sregex_iterator end;

        for (; iter != end; ++iter) {
            matches.push_back(iter->str());
        }

        // Create proper value array with match strings
        int64_t count = static_cast<int64_t>(matches.size());
        nova::runtime::ValueArray* resultArray = nova::runtime::create_value_array(count);
        resultArray->length = count;

        for (int64_t i = 0; i < count; i++) {
            char* copy = strdup(matches[i].c_str());
            resultArray->elements[i] = reinterpret_cast<int64_t>(copy);
        }

        return nova::runtime::create_metadata_from_value_array(resultArray);
    } catch (...) {
        return nullptr;
    }
}

// String.prototype.matchAll(regex) - ES2020
void* nova_string_matchAll(const char* str, void* regexPtr) {
    return nova_regex_matchAll(regexPtr, str);
}

} // extern "C"
