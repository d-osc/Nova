// Symbol.cpp - ES2015+ Symbol implementation for Nova
// Provides Symbol primitive type with well-known symbols

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <atomic>

extern "C" {

// ============================================================================
// Symbol Structure
// ============================================================================

struct NovaSymbol {
    int64_t id;           // Unique symbol ID
    const char* description;  // Optional description
    bool isWellKnown;     // Is this a well-known symbol?
};

// Global symbol counter for unique IDs
static std::atomic<int64_t> symbolCounter{1000};  // Start after well-known symbols

// Global symbol registry for Symbol.for()
static std::unordered_map<std::string, NovaSymbol*> globalRegistry;

// ============================================================================
// Well-known Symbol IDs (predefined constants)
// ============================================================================
constexpr int64_t SYMBOL_ITERATOR = 1;
constexpr int64_t SYMBOL_ASYNC_ITERATOR = 2;
constexpr int64_t SYMBOL_HAS_INSTANCE = 3;
constexpr int64_t SYMBOL_IS_CONCAT_SPREADABLE = 4;
constexpr int64_t SYMBOL_MATCH = 5;
constexpr int64_t SYMBOL_MATCH_ALL = 6;
constexpr int64_t SYMBOL_REPLACE = 7;
constexpr int64_t SYMBOL_SEARCH = 8;
constexpr int64_t SYMBOL_SPECIES = 9;
constexpr int64_t SYMBOL_SPLIT = 10;
constexpr int64_t SYMBOL_TO_PRIMITIVE = 11;
constexpr int64_t SYMBOL_TO_STRING_TAG = 12;
constexpr int64_t SYMBOL_UNSCOPABLES = 13;
constexpr int64_t SYMBOL_DISPOSE = 14;
constexpr int64_t SYMBOL_ASYNC_DISPOSE = 15;

// ============================================================================
// Constructor
// ============================================================================

// Symbol() or Symbol(description) - Create a new unique symbol
void* nova_symbol_create(const char* description) {
    NovaSymbol* sym = new NovaSymbol();
    sym->id = symbolCounter.fetch_add(1);
    sym->description = description ? strdup(description) : nullptr;
    sym->isWellKnown = false;
    return sym;
}

// Create a well-known symbol (internal use)
static NovaSymbol* createWellKnownSymbol(int64_t id, const char* description) {
    NovaSymbol* sym = new NovaSymbol();
    sym->id = id;
    sym->description = description ? strdup(description) : nullptr;
    sym->isWellKnown = true;
    return sym;
}

// ============================================================================
// Static Methods
// ============================================================================

// Symbol.for(key) - Get or create symbol in global registry
void* nova_symbol_for(const char* key) {
    if (!key) key = "";

    std::string keyStr(key);
    auto it = globalRegistry.find(keyStr);
    if (it != globalRegistry.end()) {
        return it->second;
    }

    // Create new symbol and add to registry
    NovaSymbol* sym = new NovaSymbol();
    sym->id = symbolCounter.fetch_add(1);
    sym->description = strdup(key);
    sym->isWellKnown = false;

    globalRegistry[keyStr] = sym;
    return sym;
}

// Symbol.keyFor(sym) - Get key from global registry (returns nullptr if not found)
const char* nova_symbol_keyFor(void* symPtr) {
    if (!symPtr) return nullptr;

    NovaSymbol* sym = static_cast<NovaSymbol*>(symPtr);

    // Well-known symbols are not in the global registry
    if (sym->isWellKnown) return nullptr;

    // Search for this symbol in the registry
    for (const auto& pair : globalRegistry) {
        if (pair.second == sym) {
            return pair.first.c_str();
        }
    }

    return nullptr;
}

// ============================================================================
// Well-known Symbols (Static Properties)
// ============================================================================

// Lazy initialization of well-known symbols
static NovaSymbol* wellKnownIterator = nullptr;
static NovaSymbol* wellKnownAsyncIterator = nullptr;
static NovaSymbol* wellKnownHasInstance = nullptr;
static NovaSymbol* wellKnownIsConcatSpreadable = nullptr;
static NovaSymbol* wellKnownMatch = nullptr;
static NovaSymbol* wellKnownMatchAll = nullptr;
static NovaSymbol* wellKnownReplace = nullptr;
static NovaSymbol* wellKnownSearch = nullptr;
static NovaSymbol* wellKnownSpecies = nullptr;
static NovaSymbol* wellKnownSplit = nullptr;
static NovaSymbol* wellKnownToPrimitive = nullptr;
static NovaSymbol* wellKnownToStringTag = nullptr;
static NovaSymbol* wellKnownUnscopables = nullptr;
static NovaSymbol* wellKnownDispose = nullptr;
static NovaSymbol* wellKnownAsyncDispose = nullptr;

// Symbol.iterator (ES2015)
void* nova_symbol_iterator() {
    if (!wellKnownIterator) {
        wellKnownIterator = createWellKnownSymbol(SYMBOL_ITERATOR, "Symbol.iterator");
    }
    return wellKnownIterator;
}

// Symbol.asyncIterator (ES2018)
void* nova_symbol_asyncIterator() {
    if (!wellKnownAsyncIterator) {
        wellKnownAsyncIterator = createWellKnownSymbol(SYMBOL_ASYNC_ITERATOR, "Symbol.asyncIterator");
    }
    return wellKnownAsyncIterator;
}

// Symbol.hasInstance (ES2015)
void* nova_symbol_hasInstance() {
    if (!wellKnownHasInstance) {
        wellKnownHasInstance = createWellKnownSymbol(SYMBOL_HAS_INSTANCE, "Symbol.hasInstance");
    }
    return wellKnownHasInstance;
}

// Symbol.isConcatSpreadable (ES2015)
void* nova_symbol_isConcatSpreadable() {
    if (!wellKnownIsConcatSpreadable) {
        wellKnownIsConcatSpreadable = createWellKnownSymbol(SYMBOL_IS_CONCAT_SPREADABLE, "Symbol.isConcatSpreadable");
    }
    return wellKnownIsConcatSpreadable;
}

// Symbol.match (ES2015)
void* nova_symbol_match() {
    if (!wellKnownMatch) {
        wellKnownMatch = createWellKnownSymbol(SYMBOL_MATCH, "Symbol.match");
    }
    return wellKnownMatch;
}

// Symbol.matchAll (ES2020)
void* nova_symbol_matchAll() {
    if (!wellKnownMatchAll) {
        wellKnownMatchAll = createWellKnownSymbol(SYMBOL_MATCH_ALL, "Symbol.matchAll");
    }
    return wellKnownMatchAll;
}

// Symbol.replace (ES2015)
void* nova_symbol_replace() {
    if (!wellKnownReplace) {
        wellKnownReplace = createWellKnownSymbol(SYMBOL_REPLACE, "Symbol.replace");
    }
    return wellKnownReplace;
}

// Symbol.search (ES2015)
void* nova_symbol_search() {
    if (!wellKnownSearch) {
        wellKnownSearch = createWellKnownSymbol(SYMBOL_SEARCH, "Symbol.search");
    }
    return wellKnownSearch;
}

// Symbol.species (ES2015)
void* nova_symbol_species() {
    if (!wellKnownSpecies) {
        wellKnownSpecies = createWellKnownSymbol(SYMBOL_SPECIES, "Symbol.species");
    }
    return wellKnownSpecies;
}

// Symbol.split (ES2015)
void* nova_symbol_split() {
    if (!wellKnownSplit) {
        wellKnownSplit = createWellKnownSymbol(SYMBOL_SPLIT, "Symbol.split");
    }
    return wellKnownSplit;
}

// Symbol.toPrimitive (ES2015)
void* nova_symbol_toPrimitive() {
    if (!wellKnownToPrimitive) {
        wellKnownToPrimitive = createWellKnownSymbol(SYMBOL_TO_PRIMITIVE, "Symbol.toPrimitive");
    }
    return wellKnownToPrimitive;
}

// Symbol.toStringTag (ES2015)
void* nova_symbol_toStringTag() {
    if (!wellKnownToStringTag) {
        wellKnownToStringTag = createWellKnownSymbol(SYMBOL_TO_STRING_TAG, "Symbol.toStringTag");
    }
    return wellKnownToStringTag;
}

// Symbol.unscopables (ES2015)
void* nova_symbol_unscopables() {
    if (!wellKnownUnscopables) {
        wellKnownUnscopables = createWellKnownSymbol(SYMBOL_UNSCOPABLES, "Symbol.unscopables");
    }
    return wellKnownUnscopables;
}

// Symbol.dispose (ES2024) - moved from Disposable.cpp
void* nova_symbol_dispose_obj() {
    if (!wellKnownDispose) {
        wellKnownDispose = createWellKnownSymbol(SYMBOL_DISPOSE, "Symbol.dispose");
    }
    return wellKnownDispose;
}

// Symbol.asyncDispose (ES2024) - moved from Disposable.cpp
void* nova_symbol_asyncDispose_obj() {
    if (!wellKnownAsyncDispose) {
        wellKnownAsyncDispose = createWellKnownSymbol(SYMBOL_ASYNC_DISPOSE, "Symbol.asyncDispose");
    }
    return wellKnownAsyncDispose;
}

// ============================================================================
// Instance Methods
// ============================================================================

// Symbol.prototype.toString() - Returns "Symbol(description)"
const char* nova_symbol_toString(void* symPtr) {
    if (!symPtr) return "Symbol()";

    NovaSymbol* sym = static_cast<NovaSymbol*>(symPtr);

    // Build "Symbol(description)" or "Symbol()"
    std::string result = "Symbol(";
    if (sym->description) {
        result += sym->description;
    }
    result += ")";

    return strdup(result.c_str());
}

// Symbol.prototype.valueOf() - Returns the symbol itself (as pointer)
void* nova_symbol_valueOf(void* symPtr) {
    return symPtr;
}

// Symbol.prototype.description (ES2019) - Returns the description or undefined
const char* nova_symbol_get_description(void* symPtr) {
    if (!symPtr) return nullptr;

    NovaSymbol* sym = static_cast<NovaSymbol*>(symPtr);
    return sym->description;  // Can be nullptr (undefined)
}

// ============================================================================
// Utility Functions
// ============================================================================

// Get the unique ID of a symbol
int64_t nova_symbol_get_id(void* symPtr) {
    if (!symPtr) return 0;
    return static_cast<NovaSymbol*>(symPtr)->id;
}

// Check if two symbols are the same
int64_t nova_symbol_equals(void* sym1, void* sym2) {
    if (sym1 == sym2) return 1;
    if (!sym1 || !sym2) return 0;

    NovaSymbol* s1 = static_cast<NovaSymbol*>(sym1);
    NovaSymbol* s2 = static_cast<NovaSymbol*>(sym2);

    return s1->id == s2->id ? 1 : 0;
}

// Check if value is a symbol
int64_t nova_is_symbol(void* ptr) {
    // In practice, we would need type tagging
    // For now, return 1 if it looks like a valid symbol
    if (!ptr) return 0;
    NovaSymbol* sym = static_cast<NovaSymbol*>(ptr);
    // Check if ID is in valid range
    return (sym->id >= 1 && sym->id < symbolCounter.load()) ? 1 : 0;
}

} // extern "C"
