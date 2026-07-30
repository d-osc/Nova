#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <ctime>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <vector>
#include "nova/runtime/Value.h"

namespace nova::runtime {
struct ValueArray {
    struct {
        size_t size;
        uint32_t type_id;
        bool is_marked;
        void* next;
    } header;
    int64_t length;
    int64_t capacity;
    int64_t* elements;
};
ValueArray* create_value_array(int64_t capacity);
void* create_metadata_from_value_array(ValueArray* array);
}

extern "C" {
void* nova_dynamic_object_create();
void nova_dynamic_object_set_tagged(
    void* object, const char* key, std::uint64_t value);
}

// ============================================================================
// Intl API Implementation (Simplified for AOT Compiler)
// ============================================================================

static std::string intlOption(
        const char* options, const char* key,
        const char* fallback = "") {
    if (!options || !key) return fallback;
    const std::string source(options);
    const std::string prefix = std::string(key) + "=";
    size_t position = source.find(prefix);
    if (position == std::string::npos) return fallback;
    position += prefix.size();
    const size_t end = source.find(';', position);
    return source.substr(position, end == std::string::npos
        ? std::string::npos : end - position);
}

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
    const std::string style = intlOption(options, "style", "decimal");
    const std::string currency = intlOption(options, "currency", "USD");
    fmt->style = strdup(style.c_str());
    fmt->currency = strdup(currency.c_str());
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
        std::ostringstream formatted;
        formatted << std::fixed << std::setprecision(2) << std::fabs(value);
        std::string digits = formatted.str();
        const size_t decimal = digits.find('.');
        for (size_t position = decimal; position > 3; position -= 3) {
            digits.insert(position - 3, ",");
        }
        const char* symbol = strcmp(fmt->currency, "USD") == 0
            ? "$" : fmt->currency;
        snprintf(buffer, sizeof(buffer), "%s%s%s",
            value < 0 ? "-" : "", symbol, digits.c_str());
    } else {
        snprintf(buffer, sizeof(buffer), "%g", value);
    }

    return strdup(buffer);
}

void* nova_intl_numberformat_resolvedoptions(void* fmtPtr) {
    NovaNumberFormat* fmt = static_cast<NovaNumberFormat*>(fmtPtr);
    return strdup(fmt->locale);
}

const char* nova_intl_numberformat_currency(void* fmtPtr) {
    auto* fmt = static_cast<NovaNumberFormat*>(fmtPtr);
    return fmt && fmt->currency ? fmt->currency : "";
}

void nova_intl_numberformat_free(void* fmtPtr) {
    NovaNumberFormat* fmt = static_cast<NovaNumberFormat*>(fmtPtr);
    if (fmt) {
        free(fmt->locale);
        free(fmt->style);
        free(fmt->currency);
        free(fmt);
    }
}

// formatToParts returns JSON array of parts
void* nova_intl_numberformat_formattoparts([[maybe_unused]] void* fmtPtr, double value) {
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "[{\"type\":\"integer\",\"value\":\"%g\"}]", value);
    return strdup(buffer);
}

// formatRange for number ranges
void* nova_intl_numberformat_formatrange([[maybe_unused]] void* fmtPtr, double start, double end) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%g – %g", start, end);
    return strdup(buffer);
}

void* nova_intl_numberformat_formatrangetoparts([[maybe_unused]] void* fmtPtr, double start, double end) {
    char buffer[512];
    snprintf(buffer, sizeof(buffer),
        "[{\"type\":\"integer\",\"value\":\"%g\",\"source\":\"startRange\"},"
        "{\"type\":\"literal\",\"value\":\" – \",\"source\":\"shared\"},"
        "{\"type\":\"integer\",\"value\":\"%g\",\"source\":\"endRange\"}]", start, end);
    return strdup(buffer);
}

void* nova_intl_numberformat_supportedlocalesof(const char* locales) {
    return locales ? strdup(locales) : strdup("en");
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

void* nova_intl_datetimeformat_create(const char* locale, [[maybe_unused]] const char* options) {
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

void nova_intl_datetimeformat_free(void* fmtPtr) {
    NovaDateTimeFormat* fmt = static_cast<NovaDateTimeFormat*>(fmtPtr);
    if (fmt) {
        free(fmt->locale);
        free(fmt->dateStyle);
        free(fmt->timeStyle);
        free(fmt->timeZone);
        free(fmt);
    }
}

void* nova_intl_datetimeformat_formattoparts([[maybe_unused]] void* fmtPtr, int64_t timestamp) {
    time_t t = static_cast<time_t>(timestamp / 1000);
    struct tm* tm_info = gmtime(&t);
    auto* result = nova::runtime::create_value_array(
        tm_info ? 5 : 0);
    if (!tm_info) {
        return nova::runtime::create_metadata_from_value_array(result);
    }

    auto addPart = [&](const char* type, const std::string& value) {
        void* part = nova_dynamic_object_create();
        nova_dynamic_object_set_tagged(
            part, "type", nova_value_from_string(type));
        char* stableValue = strdup(value.c_str());
        nova_dynamic_object_set_tagged(
            part, "value", nova_value_from_string(stableValue));
        result->elements[result->length++] =
            static_cast<int64_t>(nova_value_from_object(part));
    };
    char month[3];
    char day[3];
    char year[8];
    snprintf(month, sizeof(month), "%02d", tm_info->tm_mon + 1);
    snprintf(day, sizeof(day), "%02d", tm_info->tm_mday);
    snprintf(year, sizeof(year), "%d", tm_info->tm_year + 1900);
    addPart("month", month);
    addPart("literal", "/");
    addPart("day", day);
    addPart("literal", "/");
    addPart("year", year);
    return nova::runtime::create_metadata_from_value_array(result);
}

void* nova_intl_datetimeformat_formatrange([[maybe_unused]] void* fmtPtr, int64_t start, int64_t end) {
    time_t t1 = static_cast<time_t>(start / 1000);
    time_t t2 = static_cast<time_t>(end / 1000);
    struct tm* tm1 = gmtime(&t1);
    struct tm* tm2 = gmtime(&t2);

    char buffer[256];
    if (tm1 && tm2) {
        char buf1[64], buf2[64];
        strftime(buf1, sizeof(buf1), "%m/%d/%Y", tm1);
        strftime(buf2, sizeof(buf2), "%m/%d/%Y", tm2);
        snprintf(buffer, sizeof(buffer), "%s – %s", buf1, buf2);
    } else {
        snprintf(buffer, sizeof(buffer), "Invalid Date");
    }
    return strdup(buffer);
}

void* nova_intl_datetimeformat_formatrangetoparts([[maybe_unused]] void* fmtPtr, [[maybe_unused]] int64_t start, [[maybe_unused]] int64_t end) {
    char buffer[512];
    snprintf(buffer, sizeof(buffer),
        "[{\"type\":\"month\",\"value\":\"1\",\"source\":\"startRange\"},"
        "{\"type\":\"literal\",\"value\":\" – \",\"source\":\"shared\"},"
        "{\"type\":\"month\",\"value\":\"12\",\"source\":\"endRange\"}]");
    return strdup(buffer);
}

void* nova_intl_datetimeformat_supportedlocalesof(const char* locales) {
    return locales ? strdup(locales) : strdup("en");
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
    const std::string sensitivity =
        intlOption(options, "sensitivity", "variant");
    col->sensitivity = strdup(sensitivity.c_str());
    col->numeric = 0;
    return col;
}

int64_t nova_intl_collator_compare(void* colPtr, const char* str1, const char* str2) {
    auto* collator = static_cast<NovaCollator*>(colPtr);
    std::string left = str1 ? str1 : "";
    std::string right = str2 ? str2 : "";
    if (collator && strcmp(collator->sensitivity, "base") == 0) {
        std::transform(left.begin(), left.end(), left.begin(),
            [](unsigned char value) {
                return static_cast<char>(std::tolower(value));
            });
        std::transform(right.begin(), right.end(), right.begin(),
            [](unsigned char value) {
                return static_cast<char>(std::tolower(value));
            });
    }
    int result = left.compare(right);
    if (result < 0) return -1;
    if (result > 0) return 1;
    return 0;
}

void* nova_intl_collator_resolvedoptions(void* colPtr) {
    NovaCollator* col = static_cast<NovaCollator*>(colPtr);
    return strdup(col->locale);
}

void nova_intl_collator_free(void* colPtr) {
    NovaCollator* col = static_cast<NovaCollator*>(colPtr);
    if (col) {
        free(col->locale);
        free(col->usage);
        free(col->sensitivity);
        free(col);
    }
}

void* nova_intl_collator_supportedlocalesof(const char* locales) {
    return locales ? strdup(locales) : strdup("en");
}

// ============================================================================
// Intl.PluralRules
// ============================================================================

struct NovaPluralRules {
    char* locale;
    char* type;  // "cardinal" or "ordinal"
};

void* nova_intl_pluralrules_create(const char* locale, [[maybe_unused]] const char* options) {
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

void nova_intl_pluralrules_free(void* rulesPtr) {
    NovaPluralRules* rules = static_cast<NovaPluralRules*>(rulesPtr);
    if (rules) {
        free(rules->locale);
        free(rules->type);
        free(rules);
    }
}

void* nova_intl_pluralrules_selectrange(void* rulesPtr, [[maybe_unused]] double start, double end) {
    // For ranges, return category based on end value
    return nova_intl_pluralrules_select(rulesPtr, end);
}

void* nova_intl_pluralrules_supportedlocalesof(const char* locales) {
    return locales ? strdup(locales) : strdup("en");
}

// ============================================================================
// Intl.RelativeTimeFormat
// ============================================================================

struct NovaRelativeTimeFormat {
    char* locale;
    char* style;
    char* numeric;
};

void* nova_intl_relativetimeformat_create(const char* locale, [[maybe_unused]] const char* options) {
    NovaRelativeTimeFormat* fmt = static_cast<NovaRelativeTimeFormat*>(malloc(sizeof(NovaRelativeTimeFormat)));
    fmt->locale = locale ? strdup(locale) : strdup("en");
    fmt->style = strdup("long");
    fmt->numeric = strdup("always");
    return fmt;
}

void* nova_intl_relativetimeformat_format([[maybe_unused]] void* fmtPtr, double value, const char* unit) {
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

void nova_intl_relativetimeformat_free(void* fmtPtr) {
    NovaRelativeTimeFormat* fmt = static_cast<NovaRelativeTimeFormat*>(fmtPtr);
    if (fmt) {
        free(fmt->locale);
        free(fmt->style);
        free(fmt->numeric);
        free(fmt);
    }
}

void* nova_intl_relativetimeformat_formattoparts([[maybe_unused]] void* fmtPtr, double value, const char* unit) {
    int64_t absVal = static_cast<int64_t>(value < 0 ? -value : value);
    char buffer[512];

    if (value < 0) {
        snprintf(buffer, sizeof(buffer),
            "[{\"type\":\"integer\",\"value\":\"%lld\",\"unit\":\"%s\"},"
            "{\"type\":\"literal\",\"value\":\" %s ago\"}]",
            (long long)absVal, unit ? unit : "second", unit ? unit : "second");
    } else {
        snprintf(buffer, sizeof(buffer),
            "[{\"type\":\"literal\",\"value\":\"in \"},"
            "{\"type\":\"integer\",\"value\":\"%lld\",\"unit\":\"%s\"}]",
            (long long)absVal, unit ? unit : "second");
    }
    return strdup(buffer);
}

void* nova_intl_relativetimeformat_resolvedoptions(void* fmtPtr) {
    NovaRelativeTimeFormat* fmt = static_cast<NovaRelativeTimeFormat*>(fmtPtr);
    return strdup(fmt->locale);
}

void* nova_intl_relativetimeformat_supportedlocalesof(const char* locales) {
    return locales ? strdup(locales) : strdup("en");
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
    const std::string type =
        intlOption(options, "type", "conjunction");
    const std::string style =
        intlOption(options, "style", "long");
    fmt->type = strdup(type.c_str());
    fmt->style = strdup(style.c_str());
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

void* nova_intl_listformat_format(void* fmtPtr, void* arrayPtr) {
    auto* fmt = static_cast<NovaListFormat*>(fmtPtr);
    struct StringArrayMetadata {
        char header[24];
        int64_t length;
        int64_t capacity;
        const char** elements;
    };
    auto* array = static_cast<StringArrayMetadata*>(arrayPtr);
    if (!array || array->length <= 0 || !array->elements) {
        return strdup("");
    }
    const std::string conjunction =
        fmt && strcmp(fmt->type, "disjunction") == 0
        ? "or" : "and";
    std::string result;
    for (int64_t index = 0; index < array->length; ++index) {
        if (index > 0) {
            if (index == array->length - 1) {
                result += array->length > 2 ? ", " : " ";
                result += conjunction + " ";
            } else {
                result += ", ";
            }
        }
        result += array->elements[index]
            ? array->elements[index] : "";
    }
    return strdup(result.c_str());
}

void nova_intl_listformat_free(void* fmtPtr) {
    NovaListFormat* fmt = static_cast<NovaListFormat*>(fmtPtr);
    if (fmt) {
        free(fmt->locale);
        free(fmt->type);
        free(fmt->style);
        free(fmt);
    }
}

void* nova_intl_listformat_formattoparts(void* fmtPtr, const char* item1, const char* item2) {
    NovaListFormat* fmt = static_cast<NovaListFormat*>(fmtPtr);
    const char* conj = strcmp(fmt->type, "disjunction") == 0 ? " or " : " and ";

    char buffer[512];
    snprintf(buffer, sizeof(buffer),
        "[{\"type\":\"element\",\"value\":\"%s\"},"
        "{\"type\":\"literal\",\"value\":\"%s\"},"
        "{\"type\":\"element\",\"value\":\"%s\"}]",
        item1 ? item1 : "", conj, item2 ? item2 : "");
    return strdup(buffer);
}

void* nova_intl_listformat_resolvedoptions(void* fmtPtr) {
    NovaListFormat* fmt = static_cast<NovaListFormat*>(fmtPtr);
    return strdup(fmt->locale);
}

void* nova_intl_listformat_supportedlocalesof(const char* locales) {
    return locales ? strdup(locales) : strdup("en");
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

void nova_intl_displaynames_free(void* dnPtr) {
    NovaDisplayNames* dn = static_cast<NovaDisplayNames*>(dnPtr);
    if (dn) {
        free(dn->locale);
        free(dn->type);
        free(dn);
    }
}

void* nova_intl_displaynames_resolvedoptions(void* dnPtr) {
    NovaDisplayNames* dn = static_cast<NovaDisplayNames*>(dnPtr);
    return strdup(dn->locale);
}

void* nova_intl_displaynames_supportedlocalesof(const char* locales) {
    return locales ? strdup(locales) : strdup("en");
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

void nova_intl_locale_free(void* locPtr) {
    NovaLocale* loc = static_cast<NovaLocale*>(locPtr);
    if (loc) {
        free(loc->baseName);
        free(loc->language);
        free(loc->region);
        free(loc->script);
        free(loc);
    }
}

void* nova_intl_locale_get_script(void* locPtr) {
    NovaLocale* loc = static_cast<NovaLocale*>(locPtr);
    return strdup(loc->script);
}

void* nova_intl_locale_maximize(void* locPtr) {
    NovaLocale* loc = static_cast<NovaLocale*>(locPtr);
    // Simplified: add default script and region if missing
    std::string result = loc->language;
    if (loc->script[0]) {
        result += "-";
        result += loc->script;
    } else if (strcmp(loc->language, "en") == 0) {
        result += "-Latn";
    }
    if (loc->region[0]) {
        result += "-";
        result += loc->region;
    } else if (strcmp(loc->language, "en") == 0) {
        result += "-US";
    }
    return strdup(result.c_str());
}

void* nova_intl_locale_minimize(void* locPtr) {
    NovaLocale* loc = static_cast<NovaLocale*>(locPtr);
    // Simplified: just return language
    return strdup(loc->language);
}

void* nova_intl_locale_get_calendar([[maybe_unused]] void* locPtr) {
    return strdup("gregory");
}

void* nova_intl_locale_get_casefirst([[maybe_unused]] void* locPtr) {
    return strdup("false");
}

void* nova_intl_locale_get_collation([[maybe_unused]] void* locPtr) {
    return strdup("default");
}

void* nova_intl_locale_get_hourcycle([[maybe_unused]] void* locPtr) {
    return strdup("h23");
}

void* nova_intl_locale_get_numberingsystem([[maybe_unused]] void* locPtr) {
    return strdup("latn");
}

int64_t nova_intl_locale_get_numeric([[maybe_unused]] void* locPtr) {
    return 0;
}

void* nova_intl_locale_get_calendars([[maybe_unused]] void* locPtr) {
    return strdup("gregory");
}

void* nova_intl_locale_get_collations([[maybe_unused]] void* locPtr) {
    return strdup("default");
}

void* nova_intl_locale_get_hourcycles([[maybe_unused]] void* locPtr) {
    return strdup("h23,h12");
}

void* nova_intl_locale_get_numberingsystems([[maybe_unused]] void* locPtr) {
    return strdup("latn");
}

void* nova_intl_locale_get_timezones([[maybe_unused]] void* locPtr) {
    return strdup("UTC");
}

void* nova_intl_locale_get_textinfo([[maybe_unused]] void* locPtr) {
    return strdup("ltr");
}

void* nova_intl_locale_get_weekinfo([[maybe_unused]] void* locPtr) {
    return strdup("{\"firstDay\":1,\"weekend\":[6,7],\"minimalDays\":1}");
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
    const std::string selected = intlOption(
        granularity, "granularity",
        granularity ? granularity : "grapheme");
    seg->granularity = strdup(selected.c_str());
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

void nova_intl_segmenter_free(void* segPtr) {
    NovaSegmenter* seg = static_cast<NovaSegmenter*>(segPtr);
    if (seg) {
        free(seg->locale);
        free(seg->granularity);
        free(seg);
    }
}

void* nova_intl_segmenter_resolvedoptions(void* segPtr) {
    NovaSegmenter* seg = static_cast<NovaSegmenter*>(segPtr);
    return strdup(seg->locale);
}

void* nova_intl_segmenter_segment(void* segPtr, const char* str) {
    NovaSegmenter* seg = static_cast<NovaSegmenter*>(segPtr);
    const std::string input = str ? str : "";
    std::vector<std::string> segments;
    if (seg && strcmp(seg->granularity, "word") == 0) {
        std::string word;
        for (size_t i = 0; i <= input.length(); i++) {
            if (i == input.length() || input[i] == ' ' ||
                input[i] == '\t' || input[i] == '\n') {
                if (!word.empty()) {
                    segments.push_back(word);
                    word.clear();
                }
            } else {
                word += input[i];
            }
        }
    } else {
        for (char character : input) {
            segments.emplace_back(1, character);
        }
    }
    auto* result = nova::runtime::create_value_array(
        static_cast<int64_t>(segments.size()));
    result->length = static_cast<int64_t>(segments.size());
    for (size_t index = 0; index < segments.size(); ++index) {
        result->elements[index] = reinterpret_cast<int64_t>(
            strdup(segments[index].c_str()));
    }
    return nova::runtime::create_metadata_from_value_array(result);
}

void* nova_intl_segmenter_supportedlocalesof(const char* locales) {
    return locales ? strdup(locales) : strdup("en");
}

// ============================================================================
// Intl.DurationFormat (ES2023)
// ============================================================================

struct NovaDurationFormat {
    char* locale;
    char* style;  // "long", "short", "narrow", "digital"
};

void* nova_intl_durationformat_create(const char* locale, const char* style) {
    NovaDurationFormat* fmt = static_cast<NovaDurationFormat*>(malloc(sizeof(NovaDurationFormat)));
    fmt->locale = locale ? strdup(locale) : strdup("en");
    fmt->style = style ? strdup(style) : strdup("short");
    return fmt;
}

void nova_intl_durationformat_free(void* fmtPtr) {
    NovaDurationFormat* fmt = static_cast<NovaDurationFormat*>(fmtPtr);
    if (fmt) {
        free(fmt->locale);
        free(fmt->style);
        free(fmt);
    }
}

void* nova_intl_durationformat_format(void* fmtPtr, int64_t hours, int64_t minutes, int64_t seconds) {
    NovaDurationFormat* fmt = static_cast<NovaDurationFormat*>(fmtPtr);
    char buffer[256];

    if (strcmp(fmt->style, "digital") == 0) {
        snprintf(buffer, sizeof(buffer), "%lld:%02lld:%02lld",
                 (long long)hours, (long long)minutes, (long long)seconds);
    } else if (strcmp(fmt->style, "narrow") == 0) {
        snprintf(buffer, sizeof(buffer), "%lldh %lldm %llds",
                 (long long)hours, (long long)minutes, (long long)seconds);
    } else if (strcmp(fmt->style, "long") == 0) {
        snprintf(buffer, sizeof(buffer), "%lld hours, %lld minutes, %lld seconds",
                 (long long)hours, (long long)minutes, (long long)seconds);
    } else {
        snprintf(buffer, sizeof(buffer), "%lld hr, %lld min, %lld sec",
                 (long long)hours, (long long)minutes, (long long)seconds);
    }

    return strdup(buffer);
}

void* nova_intl_durationformat_formattoparts([[maybe_unused]] void* fmtPtr, int64_t hours, int64_t minutes, int64_t seconds) {
    char buffer[512];
    snprintf(buffer, sizeof(buffer),
        "[{\"type\":\"hours\",\"value\":\"%lld\"},"
        "{\"type\":\"literal\",\"value\":\":\"},"
        "{\"type\":\"minutes\",\"value\":\"%lld\"},"
        "{\"type\":\"literal\",\"value\":\":\"},"
        "{\"type\":\"seconds\",\"value\":\"%lld\"}]",
        (long long)hours, (long long)minutes, (long long)seconds);
    return strdup(buffer);
}

void* nova_intl_durationformat_resolvedoptions(void* fmtPtr) {
    NovaDurationFormat* fmt = static_cast<NovaDurationFormat*>(fmtPtr);
    return strdup(fmt->locale);
}

void* nova_intl_durationformat_supportedlocalesof(const char* locales) {
    return locales ? strdup(locales) : strdup("en");
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
