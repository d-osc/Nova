// Nova Performance Hooks Module - Node.js compatible perf_hooks
// Provides Performance Timing APIs

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <cmath>
#include <algorithm>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#include <time.h>
#endif

// ============================================================================
// Internal structures
// ============================================================================

struct PerformanceEntry {
    char* name;
    char* entryType;
    double startTime;
    double duration;
    int64_t detail;
};

struct PerformanceMark : PerformanceEntry {
    // Mark-specific fields (detail is optional)
};

struct PerformanceMeasure : PerformanceEntry {
    // Measure-specific fields
};

struct PerformanceNodeTiming {
    double name;
    double entryType;
    double startTime;
    double duration;
    double nodeStart;
    double v8Start;
    double bootstrapComplete;
    double environment;
    double loopStart;
    double loopExit;
    double idleTime;
};

struct PerformanceResourceTiming : PerformanceEntry {
    char* initiatorType;
    double workerStart;
    double redirectStart;
    double redirectEnd;
    double fetchStart;
    double domainLookupStart;
    double domainLookupEnd;
    double connectStart;
    double connectEnd;
    double secureConnectionStart;
    double requestStart;
    double responseStart;
    double responseEnd;
    int64_t transferSize;
    int64_t encodedBodySize;
    int64_t decodedBodySize;
};

struct Histogram {
    int64_t* buckets;
    int bucketCount;
    int64_t min;
    int64_t max;
    double mean;
    double stddev;
    int64_t count;
    int64_t exceeds;
};

struct PerformanceObserver {
    void* callback;
    std::vector<std::string> entryTypes;
    bool buffered;
};

// Global state
static std::vector<PerformanceEntry*> performanceEntries;
static std::vector<PerformanceMark*> performanceMarks;
static std::vector<PerformanceMeasure*> performanceMeasures;
static std::vector<PerformanceResourceTiming*> resourceTimings;
static std::vector<PerformanceObserver*> observers;
static double timeOrigin = 0;
static int resourceTimingBufferSize = 250;
static PerformanceNodeTiming* nodeTiming = nullptr;

static char* allocString(const std::string& s) {
    char* result = (char*)malloc(s.size() + 1);
    if (result) {
        memcpy(result, s.c_str(), s.size() + 1);
    }
    return result;
}

static double getCurrentTime() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration<double, std::milli>(duration).count();
}

static void initTimeOrigin() {
    if (timeOrigin == 0) {
        timeOrigin = getCurrentTime();
    }
}

static void initNodeTiming() {
    if (!nodeTiming) {
        nodeTiming = new PerformanceNodeTiming();
        double now = getCurrentTime();
        nodeTiming->nodeStart = now - 100;  // Simulated
        nodeTiming->v8Start = now - 90;
        nodeTiming->bootstrapComplete = now - 50;
        nodeTiming->environment = now - 40;
        nodeTiming->loopStart = now - 10;
        nodeTiming->loopExit = -1;
        nodeTiming->idleTime = 0;
    }
}

extern "C" {

// ============================================================================
// performance object
// ============================================================================

double nova_perf_now() {
    initTimeOrigin();
    return getCurrentTime() - timeOrigin;
}

double nova_perf_timeOrigin() {
    initTimeOrigin();
    return timeOrigin;
}

char* nova_perf_toJSON() {
    initTimeOrigin();
    char buffer[256];
    snprintf(buffer, sizeof(buffer),
        "{\"timeOrigin\":%.3f}", timeOrigin);
    return allocString(buffer);
}

// ============================================================================
// performance.mark()
// ============================================================================

void* nova_perf_mark(const char* name) {
    initTimeOrigin();

    PerformanceMark* mark = new PerformanceMark();
    mark->name = allocString(name ? name : "");
    mark->entryType = allocString("mark");
    mark->startTime = nova_perf_now();
    mark->duration = 0;
    mark->detail = 0;

    performanceMarks.push_back(mark);
    performanceEntries.push_back((PerformanceEntry*)mark);

    return mark;
}

void* nova_perf_mark_with_options(const char* name, double startTime, int64_t detail) {
    initTimeOrigin();

    PerformanceMark* mark = new PerformanceMark();
    mark->name = allocString(name ? name : "");
    mark->entryType = allocString("mark");
    mark->startTime = startTime >= 0 ? startTime : nova_perf_now();
    mark->duration = 0;
    mark->detail = detail;

    performanceMarks.push_back(mark);
    performanceEntries.push_back((PerformanceEntry*)mark);

    return mark;
}

// ============================================================================
// performance.measure()
// ============================================================================

void* nova_perf_measure(const char* name, const char* startMark, const char* endMark) {
    initTimeOrigin();

    double startTime = 0;
    double endTime = nova_perf_now();

    // Find start mark
    if (startMark) {
        for (PerformanceMark* m : performanceMarks) {
            if (strcmp(m->name, startMark) == 0) {
                startTime = m->startTime;
                break;
            }
        }
    }

    // Find end mark
    if (endMark) {
        for (PerformanceMark* m : performanceMarks) {
            if (strcmp(m->name, endMark) == 0) {
                endTime = m->startTime;
                break;
            }
        }
    }

    PerformanceMeasure* measure = new PerformanceMeasure();
    measure->name = allocString(name ? name : "");
    measure->entryType = allocString("measure");
    measure->startTime = startTime;
    measure->duration = endTime - startTime;
    measure->detail = 0;

    performanceMeasures.push_back(measure);
    performanceEntries.push_back((PerformanceEntry*)measure);

    return measure;
}

void* nova_perf_measure_with_options(const char* name, double start, double duration, int64_t detail) {
    initTimeOrigin();

    PerformanceMeasure* measure = new PerformanceMeasure();
    measure->name = allocString(name ? name : "");
    measure->entryType = allocString("measure");
    measure->startTime = start;
    measure->duration = duration;
    measure->detail = detail;

    performanceMeasures.push_back(measure);
    performanceEntries.push_back((PerformanceEntry*)measure);

    return measure;
}

// ============================================================================
// performance.clearMarks() / clearMeasures()
// ============================================================================

void nova_perf_clearMarks(const char* name) {
    if (name) {
        performanceMarks.erase(
            std::remove_if(performanceMarks.begin(), performanceMarks.end(),
                [name](PerformanceMark* m) {
                    if (strcmp(m->name, name) == 0) {
                        free(m->name);
                        free(m->entryType);
                        delete m;
                        return true;
                    }
                    return false;
                }),
            performanceMarks.end());
    } else {
        for (PerformanceMark* m : performanceMarks) {
            free(m->name);
            free(m->entryType);
            delete m;
        }
        performanceMarks.clear();
    }
}

void nova_perf_clearMeasures(const char* name) {
    if (name) {
        performanceMeasures.erase(
            std::remove_if(performanceMeasures.begin(), performanceMeasures.end(),
                [name](PerformanceMeasure* m) {
                    if (strcmp(m->name, name) == 0) {
                        free(m->name);
                        free(m->entryType);
                        delete m;
                        return true;
                    }
                    return false;
                }),
            performanceMeasures.end());
    } else {
        for (PerformanceMeasure* m : performanceMeasures) {
            free(m->name);
            free(m->entryType);
            delete m;
        }
        performanceMeasures.clear();
    }
}

void nova_perf_clearResourceTimings() {
    for (PerformanceResourceTiming* r : resourceTimings) {
        free(r->name);
        free(r->entryType);
        free(r->initiatorType);
        delete r;
    }
    resourceTimings.clear();
}

// ============================================================================
// performance.getEntries()
// ============================================================================

void** nova_perf_getEntries(int* count) {
    if (!count) return nullptr;

    *count = (int)performanceEntries.size();
    if (*count == 0) return nullptr;

    void** result = (void**)malloc(sizeof(void*) * (*count));
    for (int i = 0; i < *count; i++) {
        result[i] = performanceEntries[i];
    }
    return result;
}

void** nova_perf_getEntriesByName(const char* name, const char* type, int* count) {
    if (!count) return nullptr;

    std::vector<PerformanceEntry*> matches;
    for (PerformanceEntry* e : performanceEntries) {
        if (name && strcmp(e->name, name) != 0) continue;
        if (type && strcmp(e->entryType, type) != 0) continue;
        matches.push_back(e);
    }

    *count = (int)matches.size();
    if (*count == 0) return nullptr;

    void** result = (void**)malloc(sizeof(void*) * (*count));
    for (int i = 0; i < *count; i++) {
        result[i] = matches[i];
    }
    return result;
}

void** nova_perf_getEntriesByType(const char* type, int* count) {
    return nova_perf_getEntriesByName(nullptr, type, count);
}

// ============================================================================
// PerformanceEntry accessors
// ============================================================================

char* nova_perf_Entry_name(void* entry) {
    PerformanceEntry* e = (PerformanceEntry*)entry;
    return e ? allocString(e->name) : nullptr;
}

char* nova_perf_Entry_entryType(void* entry) {
    PerformanceEntry* e = (PerformanceEntry*)entry;
    return e ? allocString(e->entryType) : nullptr;
}

double nova_perf_Entry_startTime(void* entry) {
    PerformanceEntry* e = (PerformanceEntry*)entry;
    return e ? e->startTime : 0;
}

double nova_perf_Entry_duration(void* entry) {
    PerformanceEntry* e = (PerformanceEntry*)entry;
    return e ? e->duration : 0;
}

int64_t nova_perf_Entry_detail(void* entry) {
    PerformanceEntry* e = (PerformanceEntry*)entry;
    return e ? e->detail : 0;
}

char* nova_perf_Entry_toJSON(void* entry) {
    PerformanceEntry* e = (PerformanceEntry*)entry;
    if (!e) return nullptr;

    char buffer[512];
    snprintf(buffer, sizeof(buffer),
        "{\"name\":\"%s\",\"entryType\":\"%s\",\"startTime\":%.3f,\"duration\":%.3f}",
        e->name, e->entryType, e->startTime, e->duration);
    return allocString(buffer);
}

// ============================================================================
// performance.nodeTiming
// ============================================================================

void* nova_perf_nodeTiming() {
    initNodeTiming();
    return nodeTiming;
}

double nova_perf_NodeTiming_nodeStart(void* timing) {
    PerformanceNodeTiming* t = (PerformanceNodeTiming*)timing;
    return t ? t->nodeStart : 0;
}

double nova_perf_NodeTiming_v8Start(void* timing) {
    PerformanceNodeTiming* t = (PerformanceNodeTiming*)timing;
    return t ? t->v8Start : 0;
}

double nova_perf_NodeTiming_bootstrapComplete(void* timing) {
    PerformanceNodeTiming* t = (PerformanceNodeTiming*)timing;
    return t ? t->bootstrapComplete : 0;
}

double nova_perf_NodeTiming_environment(void* timing) {
    PerformanceNodeTiming* t = (PerformanceNodeTiming*)timing;
    return t ? t->environment : 0;
}

double nova_perf_NodeTiming_loopStart(void* timing) {
    PerformanceNodeTiming* t = (PerformanceNodeTiming*)timing;
    return t ? t->loopStart : 0;
}

double nova_perf_NodeTiming_loopExit(void* timing) {
    PerformanceNodeTiming* t = (PerformanceNodeTiming*)timing;
    return t ? t->loopExit : 0;
}

double nova_perf_NodeTiming_idleTime(void* timing) {
    PerformanceNodeTiming* t = (PerformanceNodeTiming*)timing;
    return t ? t->idleTime : 0;
}

// ============================================================================
// performance.setResourceTimingBufferSize()
// ============================================================================

void nova_perf_setResourceTimingBufferSize(int size) {
    resourceTimingBufferSize = size;
}

int nova_perf_getResourceTimingBufferSize() {
    return resourceTimingBufferSize;
}

// ============================================================================
// performance.timerify()
// ============================================================================

void* nova_perf_timerify(void* fn, const char* name) {
    // Returns a wrapped function that measures execution time
    // In real implementation, this would wrap the function
    (void)fn;
    (void)name;
    return fn;  // Simplified - just return original function
}

// ============================================================================
// performance.eventLoopUtilization()
// ============================================================================

void* nova_perf_eventLoopUtilization() {
    // Returns event loop utilization statistics
    double* result = (double*)malloc(sizeof(double) * 3);
    result[0] = 0.5;  // idle
    result[1] = 0.5;  // active
    result[2] = 0.5;  // utilization
    return result;
}

void* nova_perf_eventLoopUtilization_diff(void* elu1, void* elu2) {
    double* e1 = (double*)elu1;
    double* e2 = (double*)elu2;
    double* result = (double*)malloc(sizeof(double) * 3);

    if (e1 && e2) {
        result[0] = e2[0] - e1[0];
        result[1] = e2[1] - e1[1];
        result[2] = (e2[0] + e2[1]) > 0 ? e2[1] / (e2[0] + e2[1]) : 0;
    } else {
        result[0] = result[1] = result[2] = 0;
    }
    return result;
}

// ============================================================================
// PerformanceObserver
// ============================================================================

void* nova_perf_Observer_new(void* callback) {
    PerformanceObserver* obs = new PerformanceObserver();
    obs->callback = callback;
    obs->buffered = false;
    return obs;
}

void nova_perf_Observer_free(void* observer) {
    PerformanceObserver* obs = (PerformanceObserver*)observer;
    if (obs) {
        delete obs;
    }
}

void nova_perf_Observer_observe(void* observer, const char* type, int buffered) {
    PerformanceObserver* obs = (PerformanceObserver*)observer;
    if (obs && type) {
        obs->entryTypes.push_back(type);
        obs->buffered = buffered != 0;
        observers.push_back(obs);
    }
}

void nova_perf_Observer_disconnect(void* observer) {
    PerformanceObserver* obs = (PerformanceObserver*)observer;
    if (obs) {
        observers.erase(
            std::remove(observers.begin(), observers.end(), obs),
            observers.end());
    }
}

void** nova_perf_Observer_takeRecords(void* observer, int* count) {
    // Return and clear buffered entries
    (void)observer;
    if (count) *count = 0;
    return nullptr;
}

// ============================================================================
// monitorEventLoopDelay()
// ============================================================================

void* nova_perf_monitorEventLoopDelay(int resolution) {
    Histogram* h = new Histogram();
    h->buckets = nullptr;
    h->bucketCount = 0;
    h->min = 0;
    h->max = 0;
    h->mean = 0;
    h->stddev = 0;
    h->count = 0;
    h->exceeds = 0;
    (void)resolution;
    return h;
}

void nova_perf_EventLoopDelayMonitor_enable(void* monitor) {
    (void)monitor;
}

void nova_perf_EventLoopDelayMonitor_disable(void* monitor) {
    (void)monitor;
}

void nova_perf_EventLoopDelayMonitor_reset(void* monitor) {
    Histogram* h = (Histogram*)monitor;
    if (h) {
        h->min = 0;
        h->max = 0;
        h->mean = 0;
        h->stddev = 0;
        h->count = 0;
        h->exceeds = 0;
    }
}

void nova_perf_EventLoopDelayMonitor_free(void* monitor) {
    Histogram* h = (Histogram*)monitor;
    if (h) {
        free(h->buckets);
        delete h;
    }
}

// ============================================================================
// Histogram
// ============================================================================

void* nova_perf_createHistogram(int64_t lowest, int64_t highest, int figures) {
    Histogram* h = new Histogram();
    h->buckets = nullptr;
    h->bucketCount = 0;
    h->min = lowest;
    h->max = highest;
    h->mean = 0;
    h->stddev = 0;
    h->count = 0;
    h->exceeds = 0;
    (void)figures;
    return h;
}

void nova_perf_Histogram_free(void* histogram) {
    Histogram* h = (Histogram*)histogram;
    if (h) {
        free(h->buckets);
        delete h;
    }
}

int64_t nova_perf_Histogram_min(void* histogram) {
    Histogram* h = (Histogram*)histogram;
    return h ? h->min : 0;
}

int64_t nova_perf_Histogram_max(void* histogram) {
    Histogram* h = (Histogram*)histogram;
    return h ? h->max : 0;
}

double nova_perf_Histogram_mean(void* histogram) {
    Histogram* h = (Histogram*)histogram;
    return h ? h->mean : 0;
}

double nova_perf_Histogram_stddev(void* histogram) {
    Histogram* h = (Histogram*)histogram;
    return h ? h->stddev : 0;
}

int64_t nova_perf_Histogram_count(void* histogram) {
    Histogram* h = (Histogram*)histogram;
    return h ? h->count : 0;
}

int64_t nova_perf_Histogram_exceeds(void* histogram) {
    Histogram* h = (Histogram*)histogram;
    return h ? h->exceeds : 0;
}

double nova_perf_Histogram_percentile(void* histogram, double percentile) {
    Histogram* h = (Histogram*)histogram;
    if (!h) return 0;
    // Simplified - return mean * percentile factor
    return h->mean * (percentile / 50.0);
}

void** nova_perf_Histogram_percentiles([[maybe_unused]] void* histogram, int* count) {
    if (!count) return nullptr;
    *count = 0;
    return nullptr;  // Simplified
}

void nova_perf_Histogram_record(void* histogram, int64_t value) {
    Histogram* h = (Histogram*)histogram;
    if (!h) return;

    h->count++;
    if (h->count == 1) {
        h->min = h->max = value;
        h->mean = (double)value;
    } else {
        if (value < h->min) h->min = value;
        if (value > h->max) h->max = value;
        // Running mean
        h->mean = h->mean + ((double)value - h->mean) / h->count;
    }
}

void nova_perf_Histogram_recordDelta(void* histogram) {
    // Record delta since last call
    static double lastTime = 0;
    double now = getCurrentTime();
    if (lastTime > 0) {
        nova_perf_Histogram_record(histogram, (int64_t)(now - lastTime));
    }
    lastTime = now;
}

void nova_perf_Histogram_reset(void* histogram) {
    Histogram* h = (Histogram*)histogram;
    if (h) {
        h->min = 0;
        h->max = 0;
        h->mean = 0;
        h->stddev = 0;
        h->count = 0;
        h->exceeds = 0;
    }
}

void* nova_perf_Histogram_add(void* histogram, void* other) {
    Histogram* h1 = (Histogram*)histogram;
    Histogram* h2 = (Histogram*)other;
    if (!h1 || !h2) return histogram;

    // Combine histograms
    if (h2->min < h1->min) h1->min = h2->min;
    if (h2->max > h1->max) h1->max = h2->max;
    h1->count += h2->count;
    h1->mean = (h1->mean + h2->mean) / 2;  // Simplified

    return histogram;
}

// ============================================================================
// RecordableHistogram (IntervalHistogram)
// ============================================================================

void* nova_perf_RecordableHistogram_new(int64_t lowest, int64_t highest, int figures) {
    return nova_perf_createHistogram(lowest, highest, figures);
}

void nova_perf_RecordableHistogram_free(void* histogram) {
    nova_perf_Histogram_free(histogram);
}

// ============================================================================
// Cleanup
// ============================================================================

void nova_perf_cleanup() {
    nova_perf_clearMarks(nullptr);
    nova_perf_clearMeasures(nullptr);
    nova_perf_clearResourceTimings();

    // Entries already freed by clear functions above
    performanceEntries.clear();

    for (PerformanceObserver* o : observers) {
        delete o;
    }
    observers.clear();

    if (nodeTiming) {
        delete nodeTiming;
        nodeTiming = nullptr;
    }
}

} // extern "C"
