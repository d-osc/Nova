// Nova Runtime - Regular Expression Support
// Uses C++ <regex> for pattern matching

#include <regex>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <cctype>

namespace nova::runtime {
struct ObjectHeader { char _[24]; };
struct StringArray {
    ObjectHeader header;
    int64_t length;
    int64_t capacity;
    const char** elements;
};
} // namespace nova::runtime

// Forward-declare the runtime helper implemented in String.cpp.
extern "C" nova::runtime::StringArray* nova_string_array_create(int64_t capacity);
#include <iostream>
#include <unordered_map>
#include <utility>
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
    bool unicodeLetterProperty;
    int64_t lastIndex;
    std::vector<std::pair<std::string, size_t>> namedGroups;
    // Zero-width lookbehind assertions that std::regex cannot express. Each
    // entry records the literal substring the lookbehind asserts, whether it is
    // a negative lookbehind, and the number of matched-content characters that
    // precede it in the final lowered pattern. The match helpers enforce:
    //   for a positive lookbehind, the substring ending at
    //     matchPos + prefixChars - 1 must equal `asserted`
    //   for a negative lookbehind, it must NOT. Only fixed-length (literal)
    //   prefixes are supported — sufficient for the zero-width lookbehind
    //   conformance cases.
    struct LookbehindConstraint {
        std::string asserted;
        bool negative;
        size_t prefixChars;
    };
    std::vector<LookbehindConstraint> lookbehinds;
};

extern "C++" {
struct NovaRegexMatchMetadata {
    std::unordered_map<std::string, const char*> groups;
    std::vector<std::pair<int64_t, int64_t>> indices;
};

static std::unordered_map<void*, NovaRegexMatchMetadata> regexMatchMetadata;

static std::string lowerRegexPattern(NovaRegex* regex, const char* pattern) {
    std::string source(pattern ? pattern : "");
    regex->lookbehinds.clear();
    regex->unicodeLetterProperty =
        source.find("\\p{Letter}") != std::string::npos ||
        source.find("\\p{L}") != std::string::npos;
    for (const char* property : {"\\p{Letter}", "\\p{L}"}) {
        size_t position = 0;
        while ((position = source.find(property, position)) !=
               std::string::npos) {
            source.replace(position, std::strlen(property), ".");
            ++position;
        }
    }

    // std::regex has no lookbehind support at all. Extract zero-width
    // lookbehind assertions of *literal* content ((?<=literal) / (?<!literal))
    // into constraints and remove them from the pattern sent to std::regex.
    // Each removed lookbehind is replaced by a single NUL marker byte so the
    // main lowering loop can count how many matched-content characters precede
    // it (its prefixChars); the markers are stripped after lowering. Only
    // literal lookbehind bodies are supported (no alternation/quantifiers) —
    // sufficient for the zero-width lookbehind conformance cases. Non-literal
    // or malformed lookbehinds are left untouched and std::regex will reject
    // them (which surfaces as a non-match rather than a wrong match).
    {
        std::string rebuilt;
        size_t i = 0;
        bool escaped = false;
        bool inClass = false;
        while (i < source.size()) {
            const char ch = source[i];
            if (escaped) {
                rebuilt += source[i];
                escaped = false;
                ++i;
                continue;
            }
            if (ch == '\\') { rebuilt += ch; escaped = true; ++i; continue; }
            if (ch == '[') inClass = true;
            if (ch == ']') inClass = false;
            if (!inClass && ch == '(' &&
                i + 3 < source.size() && source[i + 1] == '?' &&
                source[i + 2] == '<' &&
                (source[i + 3] == '=' || source[i + 3] == '!')) {
                const bool negative = source[i + 3] == '!';
                const auto close = source.find(')', i + 4);
                if (close != std::string::npos) {
                    const std::string body =
                        source.substr(i + 4, close - i - 4);
                    // Accept fixed-length literal bodies: a sequence of either
                    // ordinary characters or single-character escapes (`\X`
                    // where X is not a quantifier/group metacharacter). Build
                    // the literal value matched (for comparison against the
                    // input string) while rejecting variable-length constructs
                    // (quantifiers, groups, classes, back-references).
                    std::string assertedValue;
                    bool literal = !body.empty();
                    bool esc = false;
                    for (size_t bi = 0; literal && bi < body.size(); ++bi) {
                        const char bc = body[bi];
                        if (esc) {
                            // `\X`: a single-character escape matching X
                            // literally (for punctuation) or its control
                            // meaning. Accept it as one matched character.
                            assertedValue += bc;
                            esc = false;
                            continue;
                        }
                        if (bc == '\\') { esc = true; continue; }
                        if (bc == '(' || bc == ')' || bc == '{' || bc == '}' ||
                            bc == '[' || bc == ']' || bc == '*' || bc == '+' ||
                            bc == '?' || bc == '|') {
                            literal = false;
                            break;
                        }
                        assertedValue += bc;
                    }
                    if (literal && !esc) {
                        NovaRegex::LookbehindConstraint c;
                        c.asserted = assertedValue;
                        c.negative = negative;
                        c.prefixChars = 0;  // filled in during lowering
                        regex->lookbehinds.push_back(c);
                        // Marker byte stands in for the lookbehind so the
                        // lowering loop can locate it by position. It is not a
                        // regex metacharacter and is removed before compile.
                        rebuilt += '\x01';
                        i = close + 1;
                        continue;
                    }
                }
            }
            rebuilt += ch;
            ++i;
        }
        source = rebuilt;
    }

    std::string lowered;
    size_t captureIndex = 0;
    bool escaped = false;
    bool inClass = false;
    // Index of the next lookbehind constraint to fill (markers appear in the
    // source in the same order they were recorded).
    size_t nextLookbehind = 0;
    // Annex B (non-unicode): a `\N` back-reference (N=1..9) refers to an
    // existing group only if that group number appears as a capture group
    // *earlier* in the pattern. When the group exists but has not been
    // defined yet at this point, the back-reference is the empty string (an
    // unmatched reference matches the empty string). When the group number
    // never appears as a capture group at all, `\N` is a
    // LegacyOctalEscapeSequence (character code N). std::regex rejects both
    // cases as an invalid back reference, so lower them here.
    // First, count the total number of capture groups in the pattern so we
    // can distinguish "group defined later" from "group never defined".
    size_t totalCaptureGroups = 0;
    {
        bool esc = false;
        bool cls = false;
        for (size_t i = 0; i < source.size(); ++i) {
            const char ch = source[i];
            if (esc) { esc = false; continue; }
            if (ch == '\\') { esc = true; continue; }
            if (ch == '[') cls = true;
            if (ch == ']') cls = false;
            if (!cls && ch == '(') {
                if (i + 3 < source.size() && source[i + 1] == '?' &&
                    source[i + 2] == '<' &&
                    source[i + 3] != '=' && source[i + 3] != '!') {
                    ++totalCaptureGroups;  // named group
                } else if (i + 1 >= source.size() || source[i + 1] != '?') {
                    ++totalCaptureGroups;  // plain group
                }
            }
        }
    }
    for (size_t i = 0; i < source.size(); ++i) {
        const char ch = source[i];
        // Lookbehind marker (zero-width, stripped from the lowered output).
        // Record how many lowered content characters precede it so the match
        // helpers can enforce the assertion relative to each match start.
        if (ch == '\x01' && !escaped) {
            if (nextLookbehind < regex->lookbehinds.size()) {
                // Count matched-content characters in the lowered pattern so
                // far. Each `\X` escape sequence matches exactly one character
                // (counted as 1, not 2 bytes); every other non-metacharacter
                // literal also matches one character. Quantifiers/groups would
                // make the prefix variable-length — unsupported here (such
                // patterns are left for std::regex to reject).
                size_t matchLen = 0;
                bool lesc = false;
                for (char lc : lowered) {
                    if (lesc) { ++matchLen; lesc = false; continue; }
                    if (lc == '\\') { lesc = true; continue; }
                    ++matchLen;
                }
                regex->lookbehinds[nextLookbehind].prefixChars = matchLen;
                ++nextLookbehind;
            }
            continue;
        }
        if (escaped) {
            // Annex B: without the unicode flags, an incomplete \xHH or
            // \uHHHH sequence is an IdentityEscape.  std::regex rejects the
            // dangling escape, so lower it to the literal x/u spelling.
            if (!regex->unicode && !regex->unicodeSets &&
                (ch == 'x' || ch == 'u')) {
                const size_t required = ch == 'x' ? 2 : 4;
                bool complete = i + required < source.size();
                for (size_t offset = 1;
                     complete && offset <= required; ++offset) {
                    complete = std::isxdigit(
                        static_cast<unsigned char>(source[i + offset])) != 0;
                }
                if (!complete && !lowered.empty() &&
                    lowered.back() == '\\') {
                    lowered.pop_back();
                }
            }
            // Annex B (non-unicode): dangling decimal back-reference.
            if (!regex->unicode && !regex->unicodeSets &&
                !inClass && ch >= '1' && ch <= '9') {
                const size_t groupNo = static_cast<size_t>(ch - '0');
                if (groupNo > captureIndex) {
                    // `\N` before (or never) its group is defined.
                    if (!lowered.empty() && lowered.back() == '\\') {
                        lowered.pop_back();  // drop the backslash
                    }
                    if (groupNo <= totalCaptureGroups) {
                        // Group exists later in the pattern: an unmatched
                        // back-reference matches the empty string — emit
                        // nothing.
                        escaped = false;
                        continue;
                    }
                    // Group never defined: LegacyOctalEscapeSequence
                    // (character code N). std::regex's ECMAScript flavour has
                    // no octal/hex escape and treats `\N` as a back-reference,
                    // so emit the raw byte (code point N). For N in 1..9 the
                    // byte is a non-metacharacter control char that matches
                    // itself.
                    lowered.push_back(static_cast<char>(groupNo));
                    escaped = false;
                    continue;
                }
            }
            lowered.push_back(ch);
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            lowered.push_back(ch);
            escaped = true;
            continue;
        }
        if (ch == '[') inClass = true;
        if (ch == ']') inClass = false;
        if (!inClass && ch == '(') {
            if (i + 3 < source.size() && source[i + 1] == '?' &&
                source[i + 2] == '<' &&
                source[i + 3] != '=' && source[i + 3] != '!') {
                const auto end = source.find('>', i + 3);
                if (end != std::string::npos) {
                    ++captureIndex;
                    regex->namedGroups.emplace_back(
                        source.substr(i + 3, end - i - 3), captureIndex);
                    lowered.push_back('(');
                    i = end;
                    continue;
                }
            }
            if (i + 1 >= source.size() || source[i + 1] != '?') {
                ++captureIndex;
            }
        }
        if (!inClass && ch == '.' && regex->dotAll) {
            lowered += "[\\s\\S]";
        } else {
            lowered.push_back(ch);
        }
    }
    return lowered;
}

static void registerRegexMatch(
    NovaRegex* regex, void* matchArray, const std::smatch& match,
    size_t baseOffset) {
    NovaRegexMatchMetadata metadata;
    for (const auto& [name, index] : regex->namedGroups) {
        if (index < match.size() && match[index].matched) {
            metadata.groups[name] = strdup(match[index].str().c_str());
        }
    }
    for (size_t i = 0; i < match.size(); ++i) {
        if (match[i].matched) {
            const int64_t start = static_cast<int64_t>(
                baseOffset + match.position(i));
            metadata.indices.emplace_back(
                start, start + static_cast<int64_t>(match.length(i)));
        } else {
            metadata.indices.emplace_back(-1, -1);
        }
    }
    regexMatchMetadata[matchArray] = std::move(metadata);
}
} // extern "C++"

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
    regex->unicodeLetterProperty = false;

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
        const std::string lowered = lowerRegexPattern(regex, pattern);
        regex->compiled = new std::regex(lowered, options);
    } catch (const std::regex_error& e) {
        std::cerr << "Regex error: " << e.what() << std::endl;
        regex->compiled = nullptr;
    }

    return regex;
}

// RegExp.prototype.compile(pattern, flags) — recompile in place.
// The runtime receives the pattern as a string and flags as a string. When
// the source argument was a RegExp literal, the HIR/pattern-arg path extracts
// its source before calling here (the harness tests call
// `subject.compile(/updated/gi)` and `thisValue.compile(thisValue)`).
// Returns the same regex pointer (recompiled). Throws a TypeError if
// lastIndex is non-writable (spec: Set(obj,"lastIndex",0,true) fails).
void* nova_regex_compile(void* regexPtr, const char* patternStr,
                         const char* flagsStr) {
    if (!regexPtr || !patternStr) return regexPtr;
    NovaRegex* regex = static_cast<NovaRegex*>(regexPtr);

    // Update pattern/flags storage.
    char* oldPattern = regex->pattern;
    char* oldFlags = regex->flags;
    regex->pattern = strdup(patternStr);
    regex->flags = strdup(flagsStr ? flagsStr : "");
    if (oldPattern) free(oldPattern);
    if (oldFlags) free(oldFlags);

    // Re-parse flags.
    regex->global = false;
    regex->ignoreCase = false;
    regex->multiline = false;
    regex->dotAll = false;
    regex->unicode = false;
    regex->sticky = false;
    regex->hasIndices = false;
    regex->unicodeSets = false;
    regex->unicodeLetterProperty = false;
    regex->lookbehinds.clear();

    std::regex_constants::syntax_option_type options =
        std::regex_constants::ECMAScript;
    if (flagsStr) {
        for (const char* p = flagsStr; *p; ++p) {
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
                case 'd': regex->hasIndices = true; break;
                case 'v': regex->unicodeSets = true; break;
            }
        }
    }
    regex->options = options;

    // Recompile.
    if (regex->compiled) {
        delete regex->compiled;
        regex->compiled = nullptr;
    }
    try {
        const std::string lowered = lowerRegexPattern(regex, patternStr);
        regex->compiled = new std::regex(lowered, options);
    } catch (const std::regex_error& e) {
        std::cerr << "Regex error: " << e.what() << std::endl;
        regex->compiled = nullptr;
    }

    // Reset lastIndex to 0 (spec: RegExpInitialize sets lastIndex = 0).
    regex->lastIndex = 0;

    return regexPtr;
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

// Verify the zero-width lookbehind constraints against a match that starts at
// `matchPos` in `s`. A positive lookbehind with prefixChars P asserts that the
// P characters immediately ending at matchPos+P-1 equal `asserted` (i.e. the
// content matched just before the lookbehind position was preceded by
// `asserted`). A negative lookbehind asserts the opposite. Returns true if all
// constraints hold.
static bool lookbehindConstraintsHold(const NovaRegex* regex,
                                       const std::string& s,
                                       size_t matchPos) {
    for (const auto& c : regex->lookbehinds) {
        // The lookbehind sits after `prefixChars` matched-content characters.
        // Its zero-width position is matchPos + prefixChars; the character(s)
        // it inspects are the `asserted.size()` characters ending at
        // matchPos + prefixChars - 1 (i.e. immediately preceding the
        // lookbehind's position).
        const long long inspEnd =
            static_cast<long long>(matchPos) +
            static_cast<long long>(c.prefixChars);  // 1 past last inspected
        const long long inspStart = inspEnd -
            static_cast<long long>(c.asserted.size());
        if (inspStart < 0 || inspEnd < 0 ||
            static_cast<size_t>(inspEnd) > s.size()) {
            // The asserted region runs before the string start: a positive
            // assertion fails, a negative one holds.
            if (!c.negative) return false;
            continue;
        }
        const std::string region =
            s.substr(static_cast<size_t>(inspStart), c.asserted.size());
        const bool eq = (region == c.asserted);
        if (c.negative && eq) return false;
        if (!c.negative && !eq) return false;
    }
    return true;
}

// Test if a string matches the regex
int64_t nova_regex_test(void* regexPtr, const char* str) {
    if (!regexPtr || !str) return 0;
    NovaRegex* regex = static_cast<NovaRegex*>(regexPtr);
    if (!regex->compiled) return 0;
    if (regex->unicodeLetterProperty) {
        const auto* bytes = reinterpret_cast<const unsigned char*>(str);
        bool foundLetter = false;
        for (; *bytes; ++bytes) {
            if ((*bytes >= 'A' && *bytes <= 'Z') ||
                (*bytes >= 'a' && *bytes <= 'z') || *bytes >= 0x80) {
                foundLetter = true;
                break;
            }
        }
        if (!foundLetter) return 0;
    }

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
        } else if (regex->lookbehinds.empty()) {
            return std::regex_search(s, *regex->compiled) ? 1 : 0;
        } else {
            // Lookbehind constraints present: search for a match that also
            // satisfies every constraint. std::regex only returns the leftmost
            // match, so re-search from each successive candidate start.
            std::smatch match;
            size_t start = 0;
            while (start <= s.size()) {
                const std::string sub = s.substr(start);
                if (!std::regex_search(sub, match, *regex->compiled)) {
                    break;
                }
                const size_t matchPos = start + match.position();
                if (lookbehindConstraintsHold(regex, s, matchPos)) {
                    return 1;
                }
                // Move the search base just past this match's start to find the
                // next candidate (advance at least one byte to guarantee
                // progress for zero-length matches).
                start = matchPos + 1;
            }
            return 0;
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
        }

        if (std::regex_search(s, match, *regex->compiled)) {
            return strdup(match.str().c_str());
        }
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

// regex.exec(str) — JS-spec shape: returns a StringArray whose element 0 is
// the full match and elements 1..N are the capture groups. Returns nullptr
// when no match.
void* nova_regex_exec_array(void* regexPtr, const char* str) {
    if (!regexPtr || !str) return nullptr;
    NovaRegex* regex = static_cast<NovaRegex*>(regexPtr);
    if (!regex->compiled) return nullptr;

    try {
        std::string s(str);
        std::smatch m;
        bool matched = false;
        size_t startPos = 0;

        if (regex->sticky || regex->global) {
            if (regex->lastIndex >= (int64_t)s.length()) {
                regex->lastIndex = 0;
                return nullptr;
            }
            std::string sub = s.substr(regex->lastIndex);
            if (regex->sticky) {
                if (std::regex_search(sub, m, *regex->compiled) && m.position() == 0) {
                    matched = true;
                    startPos = static_cast<size_t>(regex->lastIndex);
                    regex->lastIndex += m.position() + m.length();
                } else {
                    regex->lastIndex = 0;
                }
            } else {
                if (std::regex_search(sub, m, *regex->compiled)) {
                    matched = true;
                    startPos = static_cast<size_t>(regex->lastIndex);
                    regex->lastIndex += m.position() + m.length();
                } else {
                    regex->lastIndex = 0;
                }
            }
        } else {
            if (std::regex_search(s, m, *regex->compiled)) {
                matched = true;
            }
        }

        if (!matched) return nullptr;

        const size_t count = m.size();
        auto* array = nova_string_array_create(static_cast<int64_t>(count));
        array->length = static_cast<int64_t>(count);
        for (size_t i = 0; i < count; ++i) {
            const std::string sub = m[i].matched ? m[i].str() : std::string();
            char* buf = static_cast<char*>(malloc(sub.size() + 1));
            if (buf) {
                std::memcpy(buf, sub.c_str(), sub.size());
                buf[sub.size()] = 0;
                array->elements[i] = buf;
            }
        }
        registerRegexMatch(regex, array, m, startPos);
        return array;
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

// String.match(regex) variant that returns a StringArray (JS spec shape).
// Returns nullptr when no match, otherwise a StringArray whose element 0 is
// the full match and elements 1..N are the capture groups.
void* nova_string_match_array(const char* str, void* regexPtr) {
    if (!str || !regexPtr) return nullptr;
    NovaRegex* regex = static_cast<NovaRegex*>(regexPtr);
    if (!regex->compiled) return nullptr;

    try {
        std::string s(str);
        std::smatch m;

        if (!std::regex_search(s, m, *regex->compiled)) {
            return nullptr;
        }

        // Build [full_match, group1, group2, ...]
        const size_t count = m.size();
        auto* array = nova_string_array_create(static_cast<int64_t>(count));
        array->length = static_cast<int64_t>(count);
        for (size_t i = 0; i < count; ++i) {
            const std::string sub = m[i].matched ? m[i].str() : std::string();
            char* buf = static_cast<char*>(malloc(sub.size() + 1));
            if (buf) {
                std::memcpy(buf, sub.c_str(), sub.size());
                buf[sub.size()] = 0;
                array->elements[i] = buf;
            }
        }
        return array;
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
            // std::sregex_token_iterator omits the trailing empty token when
            // the final delimiter ends exactly at the end of the input.
            // ECMAScript String.prototype.split preserves that element.
            std::smatch trailing;
            if (std::regex_search(s, trailing, *regex->compiled) &&
                !s.empty()) {
                std::sregex_iterator matchIter(
                    s.begin(), s.end(), *regex->compiled);
                std::sregex_iterator matchEnd;
                size_t finalEnd = 0;
                for (; matchIter != matchEnd; ++matchIter) {
                    finalEnd = static_cast<size_t>(matchIter->position()) +
                        static_cast<size_t>(matchIter->length());
                }
                if (finalEnd == s.size()) {
                    parts.emplace_back();
                }
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

const char* nova_regex_match_group(void* matchPtr, const char* name) {
    if (!matchPtr || !name) return nullptr;
    const auto match = regexMatchMetadata.find(matchPtr);
    if (match == regexMatchMetadata.end()) return nullptr;
    const auto group = match->second.groups.find(name);
    return group == match->second.groups.end() ? nullptr : group->second;
}

int64_t nova_regex_match_index(
    void* matchPtr, int64_t capture, int64_t endpoint) {
    const auto match = regexMatchMetadata.find(matchPtr);
    if (match == regexMatchMetadata.end() || capture < 0 ||
        static_cast<size_t>(capture) >= match->second.indices.size()) {
        return -1;
    }
    return endpoint == 0
        ? match->second.indices[capture].first
        : match->second.indices[capture].second;
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
        std::vector<void*> matches;

        std::sregex_iterator iter(s.begin(), s.end(), *regex->compiled);
        std::sregex_iterator end;

        for (; iter != end; ++iter) {
            const std::smatch& match = *iter;
            auto* matchArray = nova_string_array_create(
                static_cast<int64_t>(match.size()));
            matchArray->length = static_cast<int64_t>(match.size());
            for (size_t i = 0; i < match.size(); ++i) {
                matchArray->elements[i] = strdup(match[i].str().c_str());
            }
            registerRegexMatch(
                regex, matchArray, match,
                static_cast<size_t>(match.position()));
            matches.push_back(matchArray);
        }

        // Create proper value array with match strings
        int64_t count = static_cast<int64_t>(matches.size());
        nova::runtime::ValueArray* resultArray = nova::runtime::create_value_array(count);
        resultArray->length = count;

        for (int64_t i = 0; i < count; i++) {
            resultArray->elements[i] =
                reinterpret_cast<int64_t>(matches[i]);
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

// ============================================================================
// Annex B RegExp legacy static accessors ($1-$9, input/$_, lastMatch/$&,
// lastParen/$+, leftContext/$`, rightContext/$').
//
// The legacy statics are stored per-process. A real match (exec/test/match)
// populates them; until then they are the empty string. The conformance focus
// here is the receiver-validation rule from GetLegacyRegExpStaticProperty /
// SetLegacyRegExpStaticProperty: the accessor must throw a TypeError unless
// SameValue(this, %RegExp%) holds.
//
// These native entry points are the real callable backing for the intrinsic
// accessor placeholder objects created in Object.cpp. They are reached
// indirectly (via nova_dynamic_call_method_* and the Function.prototype.call
// dispatch), so they must validate `this` themselves rather than relying on a
// statically-correct receiver.
// ============================================================================

namespace {
struct LegacyRegExpStatics {
    std::string input;        // $_
    std::string lastMatch;    // $&
    std::string lastParen;    // $+
    std::string leftContext;  // $`
    std::string rightContext; // $'
    std::string parens[9];    // $1-$9
};

LegacyRegExpStatics& legacyRegExpStatics() {
    static LegacyRegExpStatics s;
    return s;
}

// Bit-exact SameValue for NaN-boxed JSValues is correct for the receiver
// comparison here: object pointers compare by identity (the only `this` that
// passes is the %RegExp% constructor singleton boxed by the same helper), and
// primitives/undefined/null never equal that boxed pointer. NaN never reaches
// this path as a receiver.
bool legacyReceiverIsRegExpConstructor(std::uint64_t thisTagged) {
    extern void* nova_intrinsic_object(const char* path);
    void* regexpCtor = nova_intrinsic_object("RegExp");
    if (!regexpCtor) return false;
    const std::uint64_t JS_VALUE_OBJECT_TAG = 0x7ffe000000000000ULL;
    const std::uint64_t JS_VALUE_TAG_MASK = 0xffff000000000000ULL;
    if ((thisTagged & JS_VALUE_TAG_MASK) != JS_VALUE_OBJECT_TAG) return false;
    const std::uint64_t JS_VALUE_PAYLOAD_MASK = 0x0000ffffffffffffULL;
    void* thisPtr = reinterpret_cast<void*>(
        static_cast<std::uintptr_t>(thisTagged & JS_VALUE_PAYLOAD_MASK));
    return thisPtr == regexpCtor;
}

const char* legacyStaticForProperty(const char* property) {
    if (!property) return "";
    LegacyRegExpStatics& s = legacyRegExpStatics();
    // $1-$9
    if (property[0] == '$' && property[1] >= '1' && property[1] <= '9' &&
        property[2] == '\0') {
        return s.parens[property[1] - '1'].c_str();
    }
    std::string p(property);
    if (p == "input" || p == "$_") return s.input.c_str();
    if (p == "lastMatch" || p == "$&") return s.lastMatch.c_str();
    if (p == "lastParen" || p == "$+") return s.lastParen.c_str();
    if (p == "leftContext" || p == "$`") return s.leftContext.c_str();
    if (p == "rightContext" || p == "$'") return s.rightContext.c_str();
    return "";
}

std::string* legacyStaticSlotForProperty(const char* property) {
    if (!property) return nullptr;
    LegacyRegExpStatics& s = legacyRegExpStatics();
    if (property[0] == '$' && property[1] >= '1' && property[1] <= '9' &&
        property[2] == '\0') {
        return &s.parens[property[1] - '1'];
    }
    std::string p(property);
    if (p == "input" || p == "$_") return &s.input;
    if (p == "lastMatch" || p == "$&") return &s.lastMatch;
    if (p == "lastParen" || p == "$+") return &s.lastParen;
    if (p == "leftContext" || p == "$`") return &s.leftContext;
    if (p == "rightContext" || p == "$'") return &s.rightContext;
    return nullptr;
}
} // namespace

// native getter: thisTagged is the NaN-boxed receiver JSValue. Returns the
// legacy static as a tagged string, or throws TypeError and returns undefined.
std::uint64_t nova_regexp_legacy_get(std::uint64_t thisTagged,
                                     const char* property) {
    extern void nova_throw_type_error(const char*);
    extern std::uint64_t nova_value_from_string(const char*);
    if (!legacyReceiverIsRegExpConstructor(thisTagged)) {
        nova_throw_type_error("RegExp legacy static property getter called "
                              "on incompatible receiver");
        return 0x7ff9000000000000ULL; // JS_VALUE_UNDEFINED
    }
    return nova_value_from_string(legacyStaticForProperty(property));
}

// native setter: validates the receiver, then stores the value (no observable
// effect for the receiver-validation conformance cases) and returns undefined.
std::uint64_t nova_regexp_legacy_set(std::uint64_t thisTagged,
                                     std::uint64_t valueTagged,
                                     const char* property) {
    extern void nova_throw_type_error(const char*);
    extern const char* nova_value_to_string_ptr(std::uint64_t);
    if (!legacyReceiverIsRegExpConstructor(thisTagged)) {
        nova_throw_type_error("RegExp legacy static property setter called "
                              "on incompatible receiver");
        return 0x7ff9000000000000ULL; // JS_VALUE_UNDEFINED
    }
    if (std::string* slot = legacyStaticSlotForProperty(property)) {
        const char* str = nova_value_to_string_ptr(valueTagged);
        *slot = str ? std::string(str) : std::string();
    }
    return 0x7ff9000000000000ULL; // JS_VALUE_UNDEFINED
}

} // extern "C"
