// Date runtime functions for Nova compiler
// Implements ES5/ES2015 Date object

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <chrono>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

extern "C" {

// ============================================
// Date object structure
// ============================================
struct NovaDate {
    int64_t timestamp;  // Milliseconds since Unix epoch (Jan 1, 1970)
};

// ============================================
// Helper functions
// ============================================

// Get current time in milliseconds
static int64_t getCurrentTimeMs() {
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    // Convert from 100-nanosecond intervals since Jan 1, 1601 to milliseconds since Jan 1, 1970
    return (uli.QuadPart - 116444736000000000ULL) / 10000;
#else
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return static_cast<int64_t>(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
#endif
}

// Convert timestamp to tm structure (local time)
static struct tm* timestampToLocalTm(int64_t timestamp) {
    time_t seconds = static_cast<time_t>(timestamp / 1000);
    return localtime(&seconds);
}

// Convert timestamp to tm structure (UTC)
static struct tm* timestampToUtcTm(int64_t timestamp) {
    time_t seconds = static_cast<time_t>(timestamp / 1000);
    return gmtime(&seconds);
}

// Convert tm structure to timestamp
static int64_t tmToTimestamp(struct tm* t, int64_t ms) {
    time_t seconds = mktime(t);
    return static_cast<int64_t>(seconds) * 1000 + ms;
}

// ============================================
// Static Methods
// ============================================

// Date.now() - already exists in Utility.cpp, but we add it here too
int64_t nova_date_now() {
    return getCurrentTimeMs();
}

// Date.parse(string) - parse date string to timestamp
int64_t nova_date_parse(void* strPtr) {
    if (!strPtr) return 0;
    const char* str = static_cast<const char*>(strPtr);

    // Try to parse ISO 8601 format: YYYY-MM-DDTHH:MM:SS
    int year, month, day, hour = 0, minute = 0, second = 0;

    if (sscanf(str, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) >= 3 ||
        sscanf(str, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) >= 3 ||
        sscanf(str, "%d/%d/%d", &month, &day, &year) == 3) {

        struct tm t = {};
        t.tm_year = year - 1900;
        t.tm_mon = month - 1;
        t.tm_mday = day;
        t.tm_hour = hour;
        t.tm_min = minute;
        t.tm_sec = second;
        t.tm_isdst = -1;

        time_t seconds = mktime(&t);
        if (seconds != -1) {
            return static_cast<int64_t>(seconds) * 1000;
        }
    }

    // Return NaN equivalent (use INT64_MIN as invalid)
    return INT64_MIN;
}

// Date.UTC(year, month, day, hour, minute, second, ms) - create UTC timestamp
int64_t nova_date_UTC(int64_t year, int64_t month, int64_t day, int64_t hour, int64_t minute, int64_t second, int64_t ms) {
    struct tm t = {};

    // Handle 2-digit years (0-99 map to 1900-1999)
    if (year >= 0 && year <= 99) {
        t.tm_year = static_cast<int>(year);
    } else {
        t.tm_year = static_cast<int>(year - 1900);
    }

    t.tm_mon = static_cast<int>(month);
    t.tm_mday = static_cast<int>(day > 0 ? day : 1);
    t.tm_hour = static_cast<int>(hour);
    t.tm_min = static_cast<int>(minute);
    t.tm_sec = static_cast<int>(second);

#ifdef _WIN32
    time_t seconds = _mkgmtime(&t);
#else
    time_t seconds = timegm(&t);
#endif

    return static_cast<int64_t>(seconds) * 1000 + ms;
}

// ============================================
// Constructor
// ============================================

// new Date() - current time
void* nova_date_create() {
    NovaDate* date = new NovaDate();
    date->timestamp = getCurrentTimeMs();
    return date;
}

// new Date(timestamp)
void* nova_date_create_timestamp(int64_t timestamp) {
    NovaDate* date = new NovaDate();
    date->timestamp = timestamp;
    return date;
}

// new Date(year, month, day, hour, minute, second, ms)
void* nova_date_create_parts(int64_t year, int64_t month, int64_t day, int64_t hour, int64_t minute, int64_t second, int64_t ms) {
    NovaDate* date = new NovaDate();

    struct tm t = {};

    // Handle 2-digit years
    if (year >= 0 && year <= 99) {
        t.tm_year = static_cast<int>(year);
    } else {
        t.tm_year = static_cast<int>(year - 1900);
    }

    t.tm_mon = static_cast<int>(month);
    t.tm_mday = static_cast<int>(day > 0 ? day : 1);
    t.tm_hour = static_cast<int>(hour);
    t.tm_min = static_cast<int>(minute);
    t.tm_sec = static_cast<int>(second);
    t.tm_isdst = -1;

    time_t seconds = mktime(&t);
    date->timestamp = static_cast<int64_t>(seconds) * 1000 + ms;

    return date;
}

// ============================================
// Getter Methods (Local Time)
// ============================================

int64_t nova_date_getTime(void* datePtr) {
    if (!datePtr) return 0;
    return static_cast<NovaDate*>(datePtr)->timestamp;
}

int64_t nova_date_getFullYear(void* datePtr) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToLocalTm(date->timestamp);
    return t ? t->tm_year + 1900 : 0;
}

int64_t nova_date_getMonth(void* datePtr) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToLocalTm(date->timestamp);
    return t ? t->tm_mon : 0;
}

int64_t nova_date_getDate(void* datePtr) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToLocalTm(date->timestamp);
    return t ? t->tm_mday : 0;
}

int64_t nova_date_getDay(void* datePtr) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToLocalTm(date->timestamp);
    return t ? t->tm_wday : 0;
}

int64_t nova_date_getHours(void* datePtr) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToLocalTm(date->timestamp);
    return t ? t->tm_hour : 0;
}

int64_t nova_date_getMinutes(void* datePtr) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToLocalTm(date->timestamp);
    return t ? t->tm_min : 0;
}

int64_t nova_date_getSeconds(void* datePtr) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToLocalTm(date->timestamp);
    return t ? t->tm_sec : 0;
}

int64_t nova_date_getMilliseconds(void* datePtr) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    return date->timestamp % 1000;
}

int64_t nova_date_getTimezoneOffset(void* datePtr) {
    // Returns offset in minutes (UTC - local)
    time_t now = time(nullptr);
    struct tm local = *localtime(&now);
    struct tm utc = *gmtime(&now);

    int64_t localMinutes = local.tm_hour * 60 + local.tm_min;
    int64_t utcMinutes = utc.tm_hour * 60 + utc.tm_min;

    // Handle day difference
    if (local.tm_mday != utc.tm_mday) {
        if (local.tm_mday > utc.tm_mday || (local.tm_mday == 1 && utc.tm_mday > 1)) {
            localMinutes += 24 * 60;
        } else {
            utcMinutes += 24 * 60;
        }
    }

    return utcMinutes - localMinutes;
}

// ============================================
// Getter Methods (UTC)
// ============================================

int64_t nova_date_getUTCFullYear(void* datePtr) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToUtcTm(date->timestamp);
    return t ? t->tm_year + 1900 : 0;
}

int64_t nova_date_getUTCMonth(void* datePtr) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToUtcTm(date->timestamp);
    return t ? t->tm_mon : 0;
}

int64_t nova_date_getUTCDate(void* datePtr) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToUtcTm(date->timestamp);
    return t ? t->tm_mday : 0;
}

int64_t nova_date_getUTCDay(void* datePtr) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToUtcTm(date->timestamp);
    return t ? t->tm_wday : 0;
}

int64_t nova_date_getUTCHours(void* datePtr) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToUtcTm(date->timestamp);
    return t ? t->tm_hour : 0;
}

int64_t nova_date_getUTCMinutes(void* datePtr) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToUtcTm(date->timestamp);
    return t ? t->tm_min : 0;
}

int64_t nova_date_getUTCSeconds(void* datePtr) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToUtcTm(date->timestamp);
    return t ? t->tm_sec : 0;
}

int64_t nova_date_getUTCMilliseconds(void* datePtr) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    return date->timestamp % 1000;
}

// ============================================
// Setter Methods (Local Time)
// ============================================

int64_t nova_date_setTime(void* datePtr, int64_t timestamp) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    date->timestamp = timestamp;
    return timestamp;
}

int64_t nova_date_setFullYear(void* datePtr, int64_t year, int64_t month, int64_t day) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToLocalTm(date->timestamp);
    if (!t) return 0;

    struct tm newTime = *t;
    newTime.tm_year = static_cast<int>(year - 1900);
    if (month >= 0) newTime.tm_mon = static_cast<int>(month);
    if (day >= 0) newTime.tm_mday = static_cast<int>(day);
    newTime.tm_isdst = -1;

    int64_t ms = date->timestamp % 1000;
    date->timestamp = tmToTimestamp(&newTime, ms);
    return date->timestamp;
}

int64_t nova_date_setMonth(void* datePtr, int64_t month, int64_t day) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToLocalTm(date->timestamp);
    if (!t) return 0;

    struct tm newTime = *t;
    newTime.tm_mon = static_cast<int>(month);
    if (day >= 0) newTime.tm_mday = static_cast<int>(day);
    newTime.tm_isdst = -1;

    int64_t ms = date->timestamp % 1000;
    date->timestamp = tmToTimestamp(&newTime, ms);
    return date->timestamp;
}

int64_t nova_date_setDate(void* datePtr, int64_t day) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToLocalTm(date->timestamp);
    if (!t) return 0;

    struct tm newTime = *t;
    newTime.tm_mday = static_cast<int>(day);
    newTime.tm_isdst = -1;

    int64_t ms = date->timestamp % 1000;
    date->timestamp = tmToTimestamp(&newTime, ms);
    return date->timestamp;
}

int64_t nova_date_setHours(void* datePtr, int64_t hours, int64_t minutes, int64_t seconds, int64_t ms) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToLocalTm(date->timestamp);
    if (!t) return 0;

    struct tm newTime = *t;
    newTime.tm_hour = static_cast<int>(hours);
    if (minutes >= 0) newTime.tm_min = static_cast<int>(minutes);
    if (seconds >= 0) newTime.tm_sec = static_cast<int>(seconds);
    newTime.tm_isdst = -1;

    int64_t newMs = (ms >= 0) ? ms : (date->timestamp % 1000);
    date->timestamp = tmToTimestamp(&newTime, newMs);
    return date->timestamp;
}

int64_t nova_date_setMinutes(void* datePtr, int64_t minutes, int64_t seconds, int64_t ms) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToLocalTm(date->timestamp);
    if (!t) return 0;

    struct tm newTime = *t;
    newTime.tm_min = static_cast<int>(minutes);
    if (seconds >= 0) newTime.tm_sec = static_cast<int>(seconds);
    newTime.tm_isdst = -1;

    int64_t newMs = (ms >= 0) ? ms : (date->timestamp % 1000);
    date->timestamp = tmToTimestamp(&newTime, newMs);
    return date->timestamp;
}

int64_t nova_date_setSeconds(void* datePtr, int64_t seconds, int64_t ms) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToLocalTm(date->timestamp);
    if (!t) return 0;

    struct tm newTime = *t;
    newTime.tm_sec = static_cast<int>(seconds);
    newTime.tm_isdst = -1;

    int64_t newMs = (ms >= 0) ? ms : (date->timestamp % 1000);
    date->timestamp = tmToTimestamp(&newTime, newMs);
    return date->timestamp;
}

int64_t nova_date_setMilliseconds(void* datePtr, int64_t ms) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);

    // Keep the seconds part, replace milliseconds
    int64_t secondsPart = (date->timestamp / 1000) * 1000;
    date->timestamp = secondsPart + (ms % 1000);
    return date->timestamp;
}

// ============================================
// Setter Methods (UTC)
// ============================================

int64_t nova_date_setUTCFullYear(void* datePtr, int64_t year, int64_t month, int64_t day) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToUtcTm(date->timestamp);
    if (!t) return 0;

    struct tm newTime = *t;
    newTime.tm_year = static_cast<int>(year - 1900);
    if (month >= 0) newTime.tm_mon = static_cast<int>(month);
    if (day >= 0) newTime.tm_mday = static_cast<int>(day);

    int64_t ms = date->timestamp % 1000;
#ifdef _WIN32
    time_t seconds = _mkgmtime(&newTime);
#else
    time_t seconds = timegm(&newTime);
#endif
    date->timestamp = static_cast<int64_t>(seconds) * 1000 + ms;
    return date->timestamp;
}

int64_t nova_date_setUTCMonth(void* datePtr, int64_t month, int64_t day) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToUtcTm(date->timestamp);
    if (!t) return 0;

    struct tm newTime = *t;
    newTime.tm_mon = static_cast<int>(month);
    if (day >= 0) newTime.tm_mday = static_cast<int>(day);

    int64_t ms = date->timestamp % 1000;
#ifdef _WIN32
    time_t seconds = _mkgmtime(&newTime);
#else
    time_t seconds = timegm(&newTime);
#endif
    date->timestamp = static_cast<int64_t>(seconds) * 1000 + ms;
    return date->timestamp;
}

int64_t nova_date_setUTCDate(void* datePtr, int64_t day) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToUtcTm(date->timestamp);
    if (!t) return 0;

    struct tm newTime = *t;
    newTime.tm_mday = static_cast<int>(day);

    int64_t ms = date->timestamp % 1000;
#ifdef _WIN32
    time_t seconds = _mkgmtime(&newTime);
#else
    time_t seconds = timegm(&newTime);
#endif
    date->timestamp = static_cast<int64_t>(seconds) * 1000 + ms;
    return date->timestamp;
}

int64_t nova_date_setUTCHours(void* datePtr, int64_t hours, int64_t minutes, int64_t seconds, int64_t ms) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToUtcTm(date->timestamp);
    if (!t) return 0;

    struct tm newTime = *t;
    newTime.tm_hour = static_cast<int>(hours);
    if (minutes >= 0) newTime.tm_min = static_cast<int>(minutes);
    if (seconds >= 0) newTime.tm_sec = static_cast<int>(seconds);

    int64_t newMs = (ms >= 0) ? ms : (date->timestamp % 1000);
#ifdef _WIN32
    time_t secs = _mkgmtime(&newTime);
#else
    time_t secs = timegm(&newTime);
#endif
    date->timestamp = static_cast<int64_t>(secs) * 1000 + newMs;
    return date->timestamp;
}

int64_t nova_date_setUTCMinutes(void* datePtr, int64_t minutes, int64_t seconds, int64_t ms) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToUtcTm(date->timestamp);
    if (!t) return 0;

    struct tm newTime = *t;
    newTime.tm_min = static_cast<int>(minutes);
    if (seconds >= 0) newTime.tm_sec = static_cast<int>(seconds);

    int64_t newMs = (ms >= 0) ? ms : (date->timestamp % 1000);
#ifdef _WIN32
    time_t secs = _mkgmtime(&newTime);
#else
    time_t secs = timegm(&newTime);
#endif
    date->timestamp = static_cast<int64_t>(secs) * 1000 + newMs;
    return date->timestamp;
}

int64_t nova_date_setUTCSeconds(void* datePtr, int64_t seconds, int64_t ms) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToUtcTm(date->timestamp);
    if (!t) return 0;

    struct tm newTime = *t;
    newTime.tm_sec = static_cast<int>(seconds);

    int64_t newMs = (ms >= 0) ? ms : (date->timestamp % 1000);
#ifdef _WIN32
    time_t secs = _mkgmtime(&newTime);
#else
    time_t secs = timegm(&newTime);
#endif
    date->timestamp = static_cast<int64_t>(secs) * 1000 + newMs;
    return date->timestamp;
}

int64_t nova_date_setUTCMilliseconds(void* datePtr, int64_t ms) {
    return nova_date_setMilliseconds(datePtr, ms);
}

// ============================================
// Conversion Methods
// ============================================

// toString() - returns full date string
void* nova_date_toString(void* datePtr) {
    if (!datePtr) {
        char* result = static_cast<char*>(malloc(20));
        strcpy(result, "Invalid Date");
        return result;
    }

    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToLocalTm(date->timestamp);

    char* result = static_cast<char*>(malloc(64));
    if (t) {
        strftime(result, 64, "%a %b %d %Y %H:%M:%S GMT%z", t);
    } else {
        strcpy(result, "Invalid Date");
    }
    return result;
}

// toDateString() - returns date portion only
void* nova_date_toDateString(void* datePtr) {
    if (!datePtr) {
        char* result = static_cast<char*>(malloc(20));
        strcpy(result, "Invalid Date");
        return result;
    }

    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToLocalTm(date->timestamp);

    char* result = static_cast<char*>(malloc(32));
    if (t) {
        strftime(result, 32, "%a %b %d %Y", t);
    } else {
        strcpy(result, "Invalid Date");
    }
    return result;
}

// toTimeString() - returns time portion only
void* nova_date_toTimeString(void* datePtr) {
    if (!datePtr) {
        char* result = static_cast<char*>(malloc(20));
        strcpy(result, "Invalid Date");
        return result;
    }

    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToLocalTm(date->timestamp);

    char* result = static_cast<char*>(malloc(32));
    if (t) {
        strftime(result, 32, "%H:%M:%S GMT%z", t);
    } else {
        strcpy(result, "Invalid Date");
    }
    return result;
}

// toISOString() - returns ISO 8601 format
void* nova_date_toISOString(void* datePtr) {
    if (!datePtr) {
        char* result = static_cast<char*>(malloc(20));
        strcpy(result, "Invalid Date");
        return result;
    }

    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToUtcTm(date->timestamp);

    char* result = static_cast<char*>(malloc(32));
    if (t) {
        int ms = static_cast<int>(date->timestamp % 1000);
        snprintf(result, 32, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
                t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                t->tm_hour, t->tm_min, t->tm_sec, ms);
    } else {
        strcpy(result, "Invalid Date");
    }
    return result;
}

// toUTCString() - returns UTC string
void* nova_date_toUTCString(void* datePtr) {
    if (!datePtr) {
        char* result = static_cast<char*>(malloc(20));
        strcpy(result, "Invalid Date");
        return result;
    }

    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToUtcTm(date->timestamp);

    char* result = static_cast<char*>(malloc(40));
    if (t) {
        strftime(result, 40, "%a, %d %b %Y %H:%M:%S GMT", t);
    } else {
        strcpy(result, "Invalid Date");
    }
    return result;
}

// toJSON() - same as toISOString()
void* nova_date_toJSON(void* datePtr) {
    return nova_date_toISOString(datePtr);
}

// toLocaleDateString() - locale-specific date string
void* nova_date_toLocaleDateString(void* datePtr) {
    if (!datePtr) {
        char* result = static_cast<char*>(malloc(20));
        strcpy(result, "Invalid Date");
        return result;
    }

    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToLocalTm(date->timestamp);

    char* result = static_cast<char*>(malloc(32));
    if (t) {
        snprintf(result, 32, "%d/%d/%d", t->tm_mon + 1, t->tm_mday, t->tm_year + 1900);
    } else {
        strcpy(result, "Invalid Date");
    }
    return result;
}

// toLocaleTimeString() - locale-specific time string
void* nova_date_toLocaleTimeString(void* datePtr) {
    if (!datePtr) {
        char* result = static_cast<char*>(malloc(20));
        strcpy(result, "Invalid Date");
        return result;
    }

    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToLocalTm(date->timestamp);

    char* result = static_cast<char*>(malloc(16));
    if (t) {
        int hour12 = t->tm_hour % 12;
        if (hour12 == 0) hour12 = 12;
        const char* ampm = t->tm_hour < 12 ? "AM" : "PM";
        snprintf(result, 16, "%d:%02d:%02d %s", hour12, t->tm_min, t->tm_sec, ampm);
    } else {
        strcpy(result, "Invalid Date");
    }
    return result;
}

// toLocaleString() - locale-specific full string
void* nova_date_toLocaleString(void* datePtr) {
    if (!datePtr) {
        char* result = static_cast<char*>(malloc(20));
        strcpy(result, "Invalid Date");
        return result;
    }

    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToLocalTm(date->timestamp);

    char* result = static_cast<char*>(malloc(48));
    if (t) {
        int hour12 = t->tm_hour % 12;
        if (hour12 == 0) hour12 = 12;
        const char* ampm = t->tm_hour < 12 ? "AM" : "PM";
        snprintf(result, 48, "%d/%d/%d, %d:%02d:%02d %s",
                t->tm_mon + 1, t->tm_mday, t->tm_year + 1900,
                hour12, t->tm_min, t->tm_sec, ampm);
    } else {
        strcpy(result, "Invalid Date");
    }
    return result;
}

// valueOf() - returns timestamp
int64_t nova_date_valueOf(void* datePtr) {
    return nova_date_getTime(datePtr);
}

// getYear() - deprecated, returns year - 1900
int64_t nova_date_getYear(void* datePtr) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToLocalTm(date->timestamp);
    return t ? t->tm_year : 0;
}

// setYear() - deprecated
int64_t nova_date_setYear(void* datePtr, int64_t year) {
    if (!datePtr) return 0;
    NovaDate* date = static_cast<NovaDate*>(datePtr);
    struct tm* t = timestampToLocalTm(date->timestamp);
    if (!t) return 0;

    struct tm newTime = *t;
    // setYear uses 2-digit years for 0-99, otherwise year - 1900
    if (year >= 0 && year <= 99) {
        newTime.tm_year = static_cast<int>(year);
    } else {
        newTime.tm_year = static_cast<int>(year - 1900);
    }
    newTime.tm_isdst = -1;

    int64_t ms = date->timestamp % 1000;
    date->timestamp = tmToTimestamp(&newTime, ms);
    return date->timestamp;
}

} // extern "C"
