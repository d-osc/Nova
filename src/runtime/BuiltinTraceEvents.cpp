// Nova Runtime - Trace Events Module
// Implements Node.js-compatible trace_events functionality
// Used for capturing trace event data for performance analysis

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <fstream>
#include <sstream>
#include <atomic>

extern "C" {

// ============================================================================
// Trace Event Categories (Node.js built-in categories)
// ============================================================================

// Built-in category constants
const char* nova_trace_events_CATEGORY_NODE() { return "node"; }
const char* nova_trace_events_CATEGORY_NODE_ASYNC_HOOKS() { return "node.async_hooks"; }
const char* nova_trace_events_CATEGORY_NODE_BOOTSTRAP() { return "node.bootstrap"; }
const char* nova_trace_events_CATEGORY_NODE_CONSOLE() { return "node.console"; }
const char* nova_trace_events_CATEGORY_NODE_DNS_NATIVE() { return "node.dns.native"; }
const char* nova_trace_events_CATEGORY_NODE_ENVIRONMENT() { return "node.environment"; }
const char* nova_trace_events_CATEGORY_NODE_FS_SYNC() { return "node.fs.sync"; }
const char* nova_trace_events_CATEGORY_NODE_FS_ASYNC() { return "node.fs.async"; }
const char* nova_trace_events_CATEGORY_NODE_NET_NATIVE() { return "node.net.native"; }
const char* nova_trace_events_CATEGORY_NODE_PERF() { return "node.perf"; }
const char* nova_trace_events_CATEGORY_NODE_PERF_USERTIMING() { return "node.perf.usertiming"; }
const char* nova_trace_events_CATEGORY_NODE_PERF_TIMERIFY() { return "node.perf.timerify"; }
const char* nova_trace_events_CATEGORY_NODE_PROMISES_REJECTIONS() { return "node.promises.rejections"; }
const char* nova_trace_events_CATEGORY_NODE_VM_SCRIPT() { return "node.vm.script"; }
const char* nova_trace_events_CATEGORY_V8() { return "v8"; }

// ============================================================================
// Trace Event Structure
// ============================================================================

struct TraceEvent {
    std::string name;
    std::string category;
    char phase;  // 'B' begin, 'E' end, 'X' complete, 'I' instant, etc.
    int64_t timestamp;  // microseconds
    int64_t duration;   // for complete events
    int pid;
    int tid;
    std::string args;   // JSON string
};

// ============================================================================
// Tracing Object
// ============================================================================

struct Tracing {
    std::set<std::string> categories;
    bool enabled;
    std::vector<TraceEvent> events;
    std::mutex mutex;
    int64_t startTime;
};

static std::vector<Tracing*> g_tracings;
static std::set<std::string> g_enabledCategories;
static std::mutex g_traceMutex;
static std::atomic<bool> g_globalTracingEnabled{false};

// Get current timestamp in microseconds
static int64_t getCurrentTimestamp() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

// ============================================================================
// createTracing(options) - Create a new Tracing object
// ============================================================================

void* nova_trace_events_createTracing(const char** categories, int categoryCount) {
    Tracing* tracing = new Tracing();
    tracing->enabled = false;
    tracing->startTime = 0;

    for (int i = 0; i < categoryCount; i++) {
        if (categories[i]) {
            tracing->categories.insert(categories[i]);
        }
    }

    std::lock_guard<std::mutex> lock(g_traceMutex);
    g_tracings.push_back(tracing);

    return tracing;
}

// Create with single category string (comma-separated)
void* nova_trace_events_createTracingFromString(const char* categoriesStr) {
    Tracing* tracing = new Tracing();
    tracing->enabled = false;
    tracing->startTime = 0;

    if (categoriesStr) {
        std::string cats(categoriesStr);
        std::stringstream ss(cats);
        std::string category;
        while (std::getline(ss, category, ',')) {
            // Trim whitespace
            size_t start = category.find_first_not_of(" \t");
            size_t end = category.find_last_not_of(" \t");
            if (start != std::string::npos) {
                tracing->categories.insert(category.substr(start, end - start + 1));
            }
        }
    }

    std::lock_guard<std::mutex> lock(g_traceMutex);
    g_tracings.push_back(tracing);

    return tracing;
}

// ============================================================================
// Tracing object methods
// ============================================================================

// tracing.enable() - Enable tracing for this object's categories
void nova_trace_events_tracing_enable(void* tracingPtr) {
    if (!tracingPtr) return;
    Tracing* tracing = (Tracing*)tracingPtr;

    std::lock_guard<std::mutex> lock(tracing->mutex);
    if (tracing->enabled) return;

    tracing->enabled = true;
    tracing->startTime = getCurrentTimestamp();
    tracing->events.clear();

    // Add categories to global enabled set
    std::lock_guard<std::mutex> glock(g_traceMutex);
    for (const auto& cat : tracing->categories) {
        g_enabledCategories.insert(cat);
    }
    g_globalTracingEnabled.store(true);
}

// tracing.disable() - Disable tracing
void nova_trace_events_tracing_disable(void* tracingPtr) {
    if (!tracingPtr) return;
    Tracing* tracing = (Tracing*)tracingPtr;

    std::lock_guard<std::mutex> lock(tracing->mutex);
    if (!tracing->enabled) return;

    tracing->enabled = false;

    // Remove categories from global set
    std::lock_guard<std::mutex> glock(g_traceMutex);
    for (const auto& cat : tracing->categories) {
        g_enabledCategories.erase(cat);
    }

    // Check if any tracing is still enabled
    bool anyEnabled = false;
    for (auto* t : g_tracings) {
        if (t->enabled) {
            anyEnabled = true;
            break;
        }
    }
    g_globalTracingEnabled.store(anyEnabled);
}

// tracing.enabled - Check if tracing is enabled
int nova_trace_events_tracing_enabled(void* tracingPtr) {
    if (!tracingPtr) return 0;
    Tracing* tracing = (Tracing*)tracingPtr;
    return tracing->enabled ? 1 : 0;
}

// tracing.categories - Get categories as comma-separated string
const char* nova_trace_events_tracing_categories(void* tracingPtr) {
    static thread_local std::string result;
    result.clear();

    if (!tracingPtr) return "";
    Tracing* tracing = (Tracing*)tracingPtr;

    std::lock_guard<std::mutex> lock(tracing->mutex);
    bool first = true;
    for (const auto& cat : tracing->categories) {
        if (!first) result += ",";
        result += cat;
        first = false;
    }

    return result.c_str();
}

// Free tracing object
void nova_trace_events_tracing_free(void* tracingPtr) {
    if (!tracingPtr) return;
    Tracing* tracing = (Tracing*)tracingPtr;

    // Disable first
    nova_trace_events_tracing_disable(tracingPtr);

    // Remove from global list
    std::lock_guard<std::mutex> lock(g_traceMutex);
    auto it = std::find(g_tracings.begin(), g_tracings.end(), tracing);
    if (it != g_tracings.end()) {
        g_tracings.erase(it);
    }

    delete tracing;
}

// ============================================================================
// getEnabledCategories() - Get all currently enabled categories
// ============================================================================

const char* nova_trace_events_getEnabledCategories() {
    static thread_local std::string result;
    result.clear();

    std::lock_guard<std::mutex> lock(g_traceMutex);

    if (g_enabledCategories.empty()) {
        return "";
    }

    bool first = true;
    for (const auto& cat : g_enabledCategories) {
        if (!first) result += ",";
        result += cat;
        first = false;
    }

    return result.c_str();
}

// ============================================================================
// Trace Event Recording (internal API for runtime use)
// ============================================================================

// Check if a category is enabled
int nova_trace_events_isCategoryEnabled(const char* category) {
    if (!category || !g_globalTracingEnabled.load()) return 0;

    std::lock_guard<std::mutex> lock(g_traceMutex);
    return g_enabledCategories.count(category) > 0 ? 1 : 0;
}

// Record a trace event
void nova_trace_events_record(const char* category, const char* name, char phase,
                               const char* args) {
    if (!category || !name || !g_globalTracingEnabled.load()) return;

    std::lock_guard<std::mutex> glock(g_traceMutex);
    if (g_enabledCategories.count(category) == 0) return;

    TraceEvent event;
    event.name = name;
    event.category = category;
    event.phase = phase;
    event.timestamp = getCurrentTimestamp();
    event.duration = 0;
    event.pid = 1;  // Simplified
    event.tid = 1;
    event.args = args ? args : "{}";

    // Add to all tracings that have this category
    for (auto* tracing : g_tracings) {
        if (tracing->enabled && tracing->categories.count(category) > 0) {
            std::lock_guard<std::mutex> lock(tracing->mutex);
            tracing->events.push_back(event);
        }
    }
}

// Record begin event
void nova_trace_events_recordBegin(const char* category, const char* name, const char* args) {
    nova_trace_events_record(category, name, 'B', args);
}

// Record end event
void nova_trace_events_recordEnd(const char* category, const char* name, const char* args) {
    nova_trace_events_record(category, name, 'E', args);
}

// Record complete event (with duration)
void nova_trace_events_recordComplete(const char* category, const char* name,
                                       int64_t durationUs, const char* args) {
    if (!category || !name || !g_globalTracingEnabled.load()) return;

    std::lock_guard<std::mutex> glock(g_traceMutex);
    if (g_enabledCategories.count(category) == 0) return;

    TraceEvent event;
    event.name = name;
    event.category = category;
    event.phase = 'X';
    event.timestamp = getCurrentTimestamp() - durationUs;
    event.duration = durationUs;
    event.pid = 1;
    event.tid = 1;
    event.args = args ? args : "{}";

    for (auto* tracing : g_tracings) {
        if (tracing->enabled && tracing->categories.count(category) > 0) {
            std::lock_guard<std::mutex> lock(tracing->mutex);
            tracing->events.push_back(event);
        }
    }
}

// Record instant event
void nova_trace_events_recordInstant(const char* category, const char* name,
                                      const char* scope, const char* args) {
    if (!category || !name || !g_globalTracingEnabled.load()) return;

    std::lock_guard<std::mutex> glock(g_traceMutex);
    if (g_enabledCategories.count(category) == 0) return;

    TraceEvent event;
    event.name = name;
    event.category = category;
    event.phase = 'I';
    event.timestamp = getCurrentTimestamp();
    event.duration = 0;
    event.pid = 1;
    event.tid = 1;

    // Include scope in args
    std::string argsStr = "{\"s\":\"";
    argsStr += (scope ? scope : "g");
    argsStr += "\"";
    if (args && strlen(args) > 2) {
        argsStr += ",";
        argsStr += std::string(args).substr(1, strlen(args) - 2);
    }
    argsStr += "}";
    event.args = argsStr;

    for (auto* tracing : g_tracings) {
        if (tracing->enabled && tracing->categories.count(category) > 0) {
            std::lock_guard<std::mutex> lock(tracing->mutex);
            tracing->events.push_back(event);
        }
    }
}

// Record counter event
void nova_trace_events_recordCounter(const char* category, const char* name,
                                      int64_t value) {
    if (!category || !name || !g_globalTracingEnabled.load()) return;

    std::lock_guard<std::mutex> glock(g_traceMutex);
    if (g_enabledCategories.count(category) == 0) return;

    TraceEvent event;
    event.name = name;
    event.category = category;
    event.phase = 'C';
    event.timestamp = getCurrentTimestamp();
    event.duration = 0;
    event.pid = 1;
    event.tid = 1;

    std::stringstream ss;
    ss << "{\"value\":" << value << "}";
    event.args = ss.str();

    for (auto* tracing : g_tracings) {
        if (tracing->enabled && tracing->categories.count(category) > 0) {
            std::lock_guard<std::mutex> lock(tracing->mutex);
            tracing->events.push_back(event);
        }
    }
}

// Record async begin
void nova_trace_events_recordAsyncBegin(const char* category, const char* name,
                                         int64_t id, const char* args) {
    if (!category || !name || !g_globalTracingEnabled.load()) return;

    std::lock_guard<std::mutex> glock(g_traceMutex);
    if (g_enabledCategories.count(category) == 0) return;

    TraceEvent event;
    event.name = name;
    event.category = category;
    event.phase = 'b';
    event.timestamp = getCurrentTimestamp();
    event.duration = id;  // Store id in duration field
    event.pid = 1;
    event.tid = 1;
    event.args = args ? args : "{}";

    for (auto* tracing : g_tracings) {
        if (tracing->enabled && tracing->categories.count(category) > 0) {
            std::lock_guard<std::mutex> lock(tracing->mutex);
            tracing->events.push_back(event);
        }
    }
}

// Record async end
void nova_trace_events_recordAsyncEnd(const char* category, const char* name,
                                       int64_t id, const char* args) {
    if (!category || !name || !g_globalTracingEnabled.load()) return;

    std::lock_guard<std::mutex> glock(g_traceMutex);
    if (g_enabledCategories.count(category) == 0) return;

    TraceEvent event;
    event.name = name;
    event.category = category;
    event.phase = 'e';
    event.timestamp = getCurrentTimestamp();
    event.duration = id;
    event.pid = 1;
    event.tid = 1;
    event.args = args ? args : "{}";

    for (auto* tracing : g_tracings) {
        if (tracing->enabled && tracing->categories.count(category) > 0) {
            std::lock_guard<std::mutex> lock(tracing->mutex);
            tracing->events.push_back(event);
        }
    }
}

// ============================================================================
// Export trace data
// ============================================================================

// Get event count
int64_t nova_trace_events_tracing_eventCount(void* tracingPtr) {
    if (!tracingPtr) return 0;
    Tracing* tracing = (Tracing*)tracingPtr;
    std::lock_guard<std::mutex> lock(tracing->mutex);
    return (int64_t)tracing->events.size();
}

// Export as JSON (Chrome trace format)
const char* nova_trace_events_tracing_exportJSON(void* tracingPtr) {
    static thread_local std::string result;
    result.clear();

    if (!tracingPtr) return "[]";
    Tracing* tracing = (Tracing*)tracingPtr;

    std::lock_guard<std::mutex> lock(tracing->mutex);

    result = "{\"traceEvents\":[";

    bool first = true;
    for (const auto& event : tracing->events) {
        if (!first) result += ",";
        first = false;

        result += "{";
        result += "\"name\":\"" + event.name + "\",";
        result += "\"cat\":\"" + event.category + "\",";
        result += "\"ph\":\"";
        result += event.phase;
        result += "\",";
        result += "\"ts\":" + std::to_string(event.timestamp) + ",";
        if (event.phase == 'X') {
            result += "\"dur\":" + std::to_string(event.duration) + ",";
        }
        result += "\"pid\":" + std::to_string(event.pid) + ",";
        result += "\"tid\":" + std::to_string(event.tid) + ",";
        result += "\"args\":" + event.args;
        result += "}";
    }

    result += "]}";

    return result.c_str();
}

// Write to file
int nova_trace_events_tracing_writeToFile(void* tracingPtr, const char* filename) {
    if (!tracingPtr || !filename) return 0;

    const char* json = nova_trace_events_tracing_exportJSON(tracingPtr);

    std::ofstream file(filename);
    if (!file.is_open()) return 0;

    file << json;
    file.close();

    return 1;
}

// Clear recorded events
void nova_trace_events_tracing_clear(void* tracingPtr) {
    if (!tracingPtr) return;
    Tracing* tracing = (Tracing*)tracingPtr;
    std::lock_guard<std::mutex> lock(tracing->mutex);
    tracing->events.clear();
}

// ============================================================================
// Global trace control
// ============================================================================

// Check if any tracing is enabled globally
int nova_trace_events_isTracingEnabled() {
    return g_globalTracingEnabled.load() ? 1 : 0;
}

// Disable all tracing
void nova_trace_events_disableAll() {
    std::lock_guard<std::mutex> lock(g_traceMutex);
    for (auto* tracing : g_tracings) {
        tracing->enabled = false;
    }
    g_enabledCategories.clear();
    g_globalTracingEnabled.store(false);
}

// Get all available categories
const char* nova_trace_events_getAllCategories() {
    return "node,node.async_hooks,node.bootstrap,node.console,node.dns.native,"
           "node.environment,node.fs.sync,node.fs.async,node.net.native,"
           "node.perf,node.perf.usertiming,node.perf.timerify,"
           "node.promises.rejections,node.vm.script,v8";
}

// ============================================================================
// Cleanup
// ============================================================================

void nova_trace_events_cleanup() {
    std::lock_guard<std::mutex> lock(g_traceMutex);
    for (auto* tracing : g_tracings) {
        delete tracing;
    }
    g_tracings.clear();
    g_enabledCategories.clear();
    g_globalTracingEnabled.store(false);
}

} // extern "C"
