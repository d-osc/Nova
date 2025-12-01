#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <ctime>
#include <cmath>

// ============================================================================
// Intl API Implementation (Simplified for AOT Compiler)
// ============================================================================

extern "C" {

// ============================================================================
// Intl.NumberFormat
// ============================================================================

struct NovaNumberFormat {
    char* locale;
    char* style;           // "decimal", "currency", "percent", "unit"
    char* currency;        // Currency code (e.g., "USD")
    int64_t minimumFractionDigits;
    int64_t maximumFractionDigits;
    int64_t useGrouping;   // Boolean: use thousand separators
};

void* nova_intl_numberformat_create(const char* locale, const char* options) {
    NovaNumberFormat* fmt = static_cast<NovaNumberFormat*>(malloc(sizeof(NovaNumberFormat)));
    fmt->locale = locale ? strdup(locale) : strdup("en");
    fmt->style = strdup("decimal");
    fmt->currency = strdup("USD");
    fmt->minimumFractionDigits = 0;
    fmt->maximumFractionDigits = 3;
    fmt->useGrouping = 1;
    return fmt;
}

void* nova_intl_numberformat_format(void* fmtPtr, double value) {
    NovaNumberFormat* fmt = static_cast<NovaNumberFormat*>(fmtPtr);
    char buffer[256];

    if (strcmp(fmt->style, "percent") == 0) {
        snprintf(buffer, sizeof(buffer), "%.0f%%", value * 100);
    } else if (strcmp(fmt->style, "currency") == 0) {
        snprintf(buffer, sizeof(buffer), "$%.2f", value);
    } else {
        snprintf(buffer, sizeof(buffer), "%g", value);
    }

    return strdup(buffer);
}

void* nova_intl_numberformat_resolvedoptions(void* fmtPtr) {
    NovaNumberFormat* fmt = static_cast<NovaNumberFormat*>(fmtPtr);
    return strdup(fmt->locale);
}

// ============================================================================
// Intl.DateTimeFormat
// ============================================================================

struct NovaDateTimeFormat {
    char* locale;
    char* dateStyle;    // "full", "long", "medium", "short"
    char* timeStyle;    // "full", "long", "medium", "short"
    char* timeZone;
};

void* nova_intl_datetimeformat_create(const char* locale, const char* options) {
    NovaDateTimeFormat* fmt = static_cast<NovaDateTimeFormat*>(malloc(sizeof(NovaDateTimeFormat)));
    fmt->locale = locale ? strdup(locale) : strdup("en");
    fmt->dateStyle = strdup("medium");
    fmt->timeStyle = strdup("medium");
    fmt->timeZone = strdup("UTC");
    return fmt;
}

void* nova_intl_datetimeformat_format(void* fmtPtr, int64_t timestamp) {
    NovaDateTimeFormat* fmt = static_cast<NovaDateTimeFormat*>(fmtPtr);
    char buffer[256];

    time_t t = static_cast<time_t>(timestamp / 1000);  // Convert from milliseconds
    struct tm* tm_info = gmtime(&t);

    if (!tm_info) {
        return strdup("Invalid Date");
    }

    // Format based on dateStyle
    if (strcmp(fmt->dateStyle, "full") == 0) {
        strftime(buffer, sizeof(buffer), "%A, %B %d, %Y", tm_info);
    } else if (strcmp(fmt->dateStyle, "long") == 0) {
        strftime(buffer, sizeof(buffer), "%B %d, %Y", tm_info);
    } else if (strcmp(fmt->dateStyle, "short") == 0) {
        strftime(buffer, sizeof(buffer), "%m/%d/%y", tm_info);
    } else {
        strftime(buffer, sizeof(buffer), "%b %d, %Y", tm_info);
    }

    return strdup(buffer);
}

void* nova_intl_datetimeformat_resolvedoptions(void* fmtPtr) {
    NovaDateTimeFormat* fmt = static_cast<NovaDateTimeFormat*>(fmtPtr);
    return strdup(fmt->locale);
}

// ============================================================================
// Intl.Collator
// ============================================================================

struct NovaCollator {
    char* locale;
    char* usage;        // "sort" or "search"
    char* sensitivity;  // "base", "accent", "case", "variant"
    int64_t numeric;    // Boolean: numeric collation
};

void* nova_intl_collator_create(const char* locale, const char* options) {
    NovaCollator* col = static_cast<NovaCollator*>(malloc(sizeof(NovaCollator)));
    col->locale = locale ? strdup(locale) : strdup("en");
    col->usage = strdup("sort");
    col->sensitivity = strdup("variant");
    col->numeric = 0;
    return col;
}

int64_t nova_intl_collator_compare(void* colPtr, const char* str1, const char* str2) {
    int result = strcmp(str1 ? str1 : "", str2 ? str2 : "");
    if (result < 0) return -1;
    if (result > 0) return 1;
    return 0;
}

void* nova_intl_collator_resolvedoptions(void* colPtr) {
    NovaCollator* col = static_cast<NovaCollator*>(colPtr);
    return strdup(col->locale);
}

// ============================================================================
// Intl.PluralRules
// ============================================================================

struct NovaPluralRules {
    char* locale;
    char* type;  // "cardinal" or "ordinal"
};

void* nova_intl_pluralrules_create(const char* locale, const char* options) {
    NovaPluralRules* rules = static_cast<NovaPluralRules*>(malloc(sizeof(NovaPluralRules)));
    rules->locale = locale ? strdup(locale) : strdup("en");
    rules->type = strdup("cardinal");
    return rules;
}

void* nova_intl_pluralrules_select(void* rulesPtr, double n) {
    NovaPluralRules* rules = static_cast<NovaPluralRules*>(rulesPtr);

    if (strcmp(rules->type, "ordinal") == 0) {
        int64_t i = static_cast<int64_t>(n);
        int64_t i10 = i % 10;
        int64_t i100 = i % 100;

        if (i10 == 1 && i100 != 11) return strdup("one");
        if (i10 == 2 && i100 != 12) return strdup("two");
        if (i10 == 3 && i100 != 13) return strdup("few");
        return strdup("other");
    } else {
        if (n == 1) return strdup("one");
        return strdup("other");
    }
}

void* nova_intl_pluralrules_resolvedoptions(void* rulesPtr) {
    NovaPluralRules* rules = static_cast<NovaPluralRules*>(rulesPtr);
    return strdup(rules->locale);
}

// ============================================================================
// Intl.RelativeTimeFormat
// ============================================================================

struct NovaRelativeTimeFormat {
    char* locale;
    char* style;
    char* numeric;
};

void* nova_intl_relativetimeformat_create(const char* locale, const char* options) {
    NovaRelativeTimeFormat* fmt = static_cast<NovaRelativeTimeFormat*>(malloc(sizeof(NovaRelativeTimeFormat)));
    fmt->locale = locale ? strdup(locale) : strdup("en");
    fmt->style = strdup("long");
    fmt->numeric = strdup("always");
    return fmt;
}

void* nova_intl_relativetimeformat_format(void* fmtPtr, double value, const char* unit) {
    char buffer[256];
    int64_t absVal = static_cast<int64_t>(value < 0 ? -value : value);
    const char* unitStr = unit ? unit : "second";

    const char* unitName;
    if (strcmp(unitStr, "year") == 0 || strcmp(unitStr, "years") == 0) {
        unitName = absVal == 1 ? "year" : "years";
    } else if (strcmp(unitStr, "month") == 0 || strcmp(unitStr, "months") == 0) {
        unitName = absVal == 1 ? "month" : "months";
    } else if (strcmp(unitStr, "week") == 0 || strcmp(unitStr, "weeks") == 0) {
        unitName = absVal == 1 ? "week" : "weeks";
    } else if (strcmp(unitStr, "day") == 0 || strcmp(unitStr, "days") == 0) {
        unitName = absVal == 1 ? "day" : "days";
    } else if (strcmp(unitStr, "hour") == 0 || strcmp(unitStr, "hours") == 0) {
        unitName = absVal == 1 ? "hour" : "hours";
    } else if (strcmp(unitStr, "minute") == 0 || strcmp(unitStr, "minutes") == 0) {
        unitName = absVal == 1 ? "minute" : "minutes";
    } else {
        unitName = absVal == 1 ? "second" : "seconds";
    }

    if (value < 0) {
        snprintf(buffer, sizeof(buffer), "%lld %s ago", (long long)absVal, unitName);
    } else {
        snprintf(buffer, sizeof(buffer), "in %lld %s", (long long)absVal, unitName);
    }

    return strdup(buffer);
}

// ============================================================================
// Intl.ListFormat
// ============================================================================

struct NovaListFormat {
    char* locale;
    char* type;
    char* style;
};

void* nova_intl_listformat_create(const char* locale, const char* options) {
    NovaListFormat* fmt = static_cast<NovaListFormat*>(malloc(sizeof(NovaListFormat)));
    fmt->locale = locale ? strdup(locale) : strdup("en");
    fmt->type = strdup("conjunction");
    fmt->style = strdup("long");
    return fmt;
}

void* nova_intl_listformat_format_simple(void* fmtPtr, const char* item1, const char* item2, const char* item3) {
    NovaListFormat* fmt = static_cast<NovaListFormat*>(fmtPtr);

    std::string result;
    const char* conj = strcmp(fmt->type, "disjunction") == 0 ? " or " : " and ";

    if (item1 && item1[0]) {
        result = item1;
        if (item2 && item2[0]) {
            if (item3 && item3[0]) {
                result += ", ";
                result += item2;
                result += conj;
                result += item3;
            } else {
                result += conj;
                result += item2;
            }
        }
    }

    return strdup(result.c_str());
}

// ============================================================================
// Intl.DisplayNames
// ============================================================================

struct NovaDisplayNames {
    char* locale;
    char* type;
};

void* nova_intl_displaynames_create(const char* locale, const char* type) {
    NovaDisplayNames* dn = static_cast<NovaDisplayNames*>(malloc(sizeof(NovaDisplayNames)));
    dn->locale = locale ? strdup(locale) : strdup("en");
    dn->type = type ? strdup(type) : strdup("language");
    return dn;
}

void* nova_intl_displaynames_of(void* dnPtr, const char* code) {
    NovaDisplayNames* dn = static_cast<NovaDisplayNames*>(dnPtr);

    if (!code) return strdup("");

    if (strcmp(dn->type, "language") == 0) {
        if (strcmp(code, "en") == 0) return strdup("English");
        if (strcmp(code, "es") == 0) return strdup("Spanish");
        if (strcmp(code, "fr") == 0) return strdup("French");
        if (strcmp(code, "de") == 0) return strdup("German");
        if (strcmp(code, "ja") == 0) return strdup("Japanese");
        if (strcmp(code, "zh") == 0) return strdup("Chinese");
        if (strcmp(code, "th") == 0) return strdup("Thai");
        if (strcmp(code, "ko") == 0) return strdup("Korean");
    } else if (strcmp(dn->type, "region") == 0) {
        if (strcmp(code, "US") == 0) return strdup("United States");
        if (strcmp(code, "GB") == 0) return strdup("United Kingdom");
        if (strcmp(code, "JP") == 0) return strdup("Japan");
        if (strcmp(code, "CN") == 0) return strdup("China");
        if (strcmp(code, "TH") == 0) return strdup("Thailand");
        if (strcmp(code, "DE") == 0) return strdup("Germany");
        if (strcmp(code, "FR") == 0) return strdup("France");
    } else if (strcmp(dn->type, "currency") == 0) {
        if (strcmp(code, "USD") == 0) return strdup("US Dollar");
        if (strcmp(code, "EUR") == 0) return strdup("Euro");
        if (strcmp(code, "GBP") == 0) return strdup("British Pound");
        if (strcmp(code, "JPY") == 0) return strdup("Japanese Yen");
        if (strcmp(code, "THB") == 0) return strdup("Thai Baht");
        if (strcmp(code, "CNY") == 0) return strdup("Chinese Yuan");
    }

    return strdup(code);
}

// ============================================================================
// Intl.Locale
// ============================================================================

struct NovaLocale {
    char* baseName;
    char* language;
    char* region;
    char* script;
};

void* nova_intl_locale_create(const char* tag) {
    NovaLocale* loc = static_cast<NovaLocale*>(malloc(sizeof(NovaLocale)));
    loc->baseName = tag ? strdup(tag) : strdup("en");

    std::string tagStr = tag ? tag : "en";
    size_t dash1 = tagStr.find('-');

    if (dash1 == std::string::npos) {
        loc->language = strdup(tagStr.c_str());
        loc->region = strdup("");
        loc->script = strdup("");
    } else {
        loc->language = strdup(tagStr.substr(0, dash1).c_str());
        std::string rest = tagStr.substr(dash1 + 1);

        size_t dash2 = rest.find('-');
        if (dash2 == std::string::npos) {
            if (rest.length() == 2) {
                loc->region = strdup(rest.c_str());
                loc->script = strdup("");
            } else {
                loc->script = strdup(rest.c_str());
                loc->region = strdup("");
            }
        } else {
            loc->script = strdup(rest.substr(0, dash2).c_str());
            loc->region = strdup(rest.substr(dash2 + 1).c_str());
        }
    }

    return loc;
}

void* nova_intl_locale_get_language(void* locPtr) {
    NovaLocale* loc = static_cast<NovaLocale*>(locPtr);
    return strdup(loc->language);
}

void* nova_intl_locale_get_region(void* locPtr) {
    NovaLocale* loc = static_cast<NovaLocale*>(locPtr);
    return strdup(loc->region);
}

void* nova_intl_locale_get_basename(void* locPtr) {
    NovaLocale* loc = static_cast<NovaLocale*>(locPtr);
    return strdup(loc->baseName);
}

void* nova_intl_locale_tostring(void* locPtr) {
    NovaLocale* loc = static_cast<NovaLocale*>(locPtr);
    return strdup(loc->baseName);
}

// ============================================================================
// Intl.Segmenter
// ============================================================================

struct NovaSegmenter {
    char* locale;
    char* granularity;
};

void* nova_intl_segmenter_create(const char* locale, const char* granularity) {
    NovaSegmenter* seg = static_cast<NovaSegmenter*>(malloc(sizeof(NovaSegmenter)));
    seg->locale = locale ? strdup(locale) : strdup("en");
    seg->granularity = granularity ? strdup(granularity) : strdup("grapheme");
    return seg;
}

int64_t nova_intl_segmenter_segment_count(void* segPtr, const char* str) {
    NovaSegmenter* seg = static_cast<NovaSegmenter*>(segPtr);
    if (!str) return 0;

    std::string s = str;

    if (strcmp(seg->granularity, "word") == 0) {
        int64_t count = 0;
        bool inWord = false;
        for (char c : s) {
            if (c == ' ' || c == '\t' || c == '\n') {
                inWord = false;
            } else if (!inWord) {
                count++;
                inWord = true;
            }
        }
        return count;
    } else if (strcmp(seg->granularity, "sentence") == 0) {
        int64_t count = 0;
        for (char c : s) {
            if (c == '.' || c == '!' || c == '?') count++;
        }
        return count > 0 ? count : (s.length() > 0 ? 1 : 0);
    } else {
        return static_cast<int64_t>(s.length());
    }
}

// ============================================================================
// Intl static methods
// ============================================================================

void* nova_intl_getcanonicallocales(const char* locale) {
    return locale ? strdup(locale) : strdup("en");
}

void* nova_intl_supportedvaluesof(const char* key) {
    if (!key) return strdup("");

    if (strcmp(key, "calendar") == 0) {
        return strdup("gregory,buddhist,chinese,islamic");
    } else if (strcmp(key, "currency") == 0) {
        return strdup("USD,EUR,GBP,JPY,THB,CNY");
    } else if (strcmp(key, "numberingSystem") == 0) {
        return strdup("latn,arab,hans,thai");
    } else if (strcmp(key, "timeZone") == 0) {
        return strdup("UTC,America/New_York,Europe/London,Asia/Tokyo,Asia/Bangkok");
    }

    return strdup("");
}

} // extern "C"
