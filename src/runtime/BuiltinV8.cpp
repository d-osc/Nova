// Nova Builtin V8 Module Implementation
// Provides Node.js-compatible v8 API (compatibility layer for LLVM-based Nova)

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>
#include <ctime>
#include <chrono>
#include "nova/runtime/Runtime.h"

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#else
#include <unistd.h>
#include <sys/resource.h>
#endif

extern "C" {

// ============================================================================
// Version and Build Info
// ============================================================================

// Returns a version tag for cached data
int64_t nova_v8_cachedDataVersionTag() {
    // Return a unique tag based on Nova compiler version
    // This simulates V8's cached data version tag
    return 0x4E4F5641;  // "NOVA" in hex
}

// Get V8 version string (Nova compatibility)
const char* nova_v8_getVersion() {
    return "Nova-LLVM/1.0.0";  // Nova uses LLVM, not V8
}

// ============================================================================
// Heap Statistics
// ============================================================================

// Get current memory usage
static void getMemoryUsage(size_t* total, size_t* used, size_t* available) {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        *used = pmc.WorkingSetSize;
        *total = pmc.PeakWorkingSetSize;
        *available = pmc.PeakWorkingSetSize - pmc.WorkingSetSize;
    } else {
        *total = *used = *available = 0;
    }
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        *used = usage.ru_maxrss * 1024;  // Convert to bytes
        *total = *used * 2;  // Estimate
        *available = *total - *used;
    } else {
        *total = *used = *available = 0;
    }
#endif
}

// v8.getHeapStatistics()
const char* nova_v8_getHeapStatistics() {
    static thread_local std::string result;

    size_t total, used, available;
    getMemoryUsage(&total, &used, &available);

    std::ostringstream json;
    json << "{";
    json << "\"total_heap_size\":" << total << ",";
    json << "\"total_heap_size_executable\":" << (total / 10) << ",";
    json << "\"total_physical_size\":" << used << ",";
    json << "\"total_available_size\":" << available << ",";
    json << "\"used_heap_size\":" << used << ",";
    json << "\"heap_size_limit\":" << (total * 2) << ",";
    json << "\"malloced_memory\":" << used << ",";
    json << "\"peak_malloced_memory\":" << total << ",";
    json << "\"does_zap_garbage\":0,";
    json << "\"number_of_native_contexts\":1,";
    json << "\"number_of_detached_contexts\":0,";
    json << "\"total_global_handles_size\":" << (used / 100) << ",";
    json << "\"used_global_handles_size\":" << (used / 200) << ",";
    json << "\"external_memory\":" << (used / 50) << "}";

    result = json.str();
    return result.c_str();
}

// v8.getHeapSpaceStatistics()
const char* nova_v8_getHeapSpaceStatistics() {
    static thread_local std::string result;

    size_t total, used, available;
    getMemoryUsage(&total, &used, &available);

    std::ostringstream json;
    json << "[";

    // Simulate V8 heap spaces
    const char* spaces[] = {
        "new_space", "old_space", "code_space", "map_space", "large_object_space"
    };
    int percentages[] = {10, 60, 15, 5, 10};

    for (int i = 0; i < 5; i++) {
        if (i > 0) json << ",";
        size_t spaceSize = total * percentages[i] / 100;
        size_t spaceUsed = used * percentages[i] / 100;
        json << "{";
        json << "\"space_name\":\"" << spaces[i] << "\",";
        json << "\"space_size\":" << spaceSize << ",";
        json << "\"space_used_size\":" << spaceUsed << ",";
        json << "\"space_available_size\":" << (spaceSize - spaceUsed) << ",";
        json << "\"physical_space_size\":" << spaceUsed << "}";
    }

    json << "]";
    result = json.str();
    return result.c_str();
}

// v8.getHeapCodeStatistics()
const char* nova_v8_getHeapCodeStatistics() {
    static thread_local std::string result;

    size_t total, used, available;
    getMemoryUsage(&total, &used, &available);

    std::ostringstream json;
    json << "{";
    json << "\"code_and_metadata_size\":" << (used / 10) << ",";
    json << "\"bytecode_and_metadata_size\":" << (used / 20) << ",";
    json << "\"external_script_source_size\":" << (used / 50) << ",";
    json << "\"cpu_profiler_metadata_size\":0}";

    result = json.str();
    return result.c_str();
}

// ============================================================================
// Heap Snapshots
// ============================================================================

// v8.writeHeapSnapshot(filename)
const char* nova_v8_writeHeapSnapshot(const char* filename) {
    static thread_local std::string result;

    std::string fname;
    if (filename && strlen(filename) > 0) {
        fname = filename;
    } else {
        // Generate default filename
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        char buf[64];
        strftime(buf, sizeof(buf), "Heap-%Y%m%d-%H%M%S", localtime(&time));
#ifdef _WIN32
        fname = std::string(buf) + "." + std::to_string(GetCurrentProcessId()) + ".heapsnapshot";
#else
        fname = std::string(buf) + "." + std::to_string(getpid()) + ".heapsnapshot";
#endif
    }

    // Write a minimal heap snapshot format
    FILE* f = fopen(fname.c_str(), "w");
    if (f) {
        size_t total, used, available;
        getMemoryUsage(&total, &used, &available);

        fprintf(f, "{\n");
        fprintf(f, "  \"snapshot\": {\n");
        fprintf(f, "    \"meta\": {\n");
        fprintf(f, "      \"node_fields\": [\"type\", \"name\", \"id\", \"self_size\", \"edge_count\"],\n");
        fprintf(f, "      \"node_types\": [[\"hidden\", \"array\", \"string\", \"object\", \"code\", \"closure\", \"regexp\", \"number\", \"native\", \"synthetic\", \"concatenated string\", \"sliced string\", \"symbol\", \"bigint\"]],\n");
        fprintf(f, "      \"edge_fields\": [\"type\", \"name_or_index\", \"to_node\"],\n");
        fprintf(f, "      \"edge_types\": [[\"context\", \"element\", \"property\", \"internal\", \"hidden\", \"shortcut\", \"weak\"]]\n");
        fprintf(f, "    },\n");
        fprintf(f, "    \"node_count\": 1,\n");
        fprintf(f, "    \"edge_count\": 0,\n");
        fprintf(f, "    \"trace_function_count\": 0\n");
        fprintf(f, "  },\n");
        fprintf(f, "  \"nodes\": [0, 0, 1, %zu, 0],\n", used);
        fprintf(f, "  \"edges\": [],\n");
        fprintf(f, "  \"trace_function_infos\": [],\n");
        fprintf(f, "  \"trace_tree\": [],\n");
        fprintf(f, "  \"samples\": [],\n");
        fprintf(f, "  \"locations\": [],\n");
        fprintf(f, "  \"strings\": [\"(root)\"]\n");
        fprintf(f, "}\n");
        fclose(f);
        result = fname;
    } else {
        result = "";
    }

    return result.c_str();
}

// Set heap snapshot limit
static int64_t heapSnapshotLimit = 0;

void nova_v8_setHeapSnapshotNearHeapLimit(int64_t limit) {
    heapSnapshotLimit = limit;
}

int64_t nova_v8_getHeapSnapshotNearHeapLimit() {
    return heapSnapshotLimit;
}

// ============================================================================
// Serialization API
// ============================================================================

struct NovaSerializer {
    std::vector<uint8_t> buffer;
    size_t offset;
};

// Create serializer
void* nova_v8_Serializer_create() {
    NovaSerializer* s = new NovaSerializer();
    s->offset = 0;
    // Write header
    s->buffer.push_back(0xFF);  // Version marker
    s->buffer.push_back(0x0F);  // V8 serialization format version
    return s;
}

// Write header
void nova_v8_Serializer_writeHeader(void* serializerPtr) {
    // Header already written in create
    (void)serializerPtr;
}

// Write value (simplified - writes as string)
void nova_v8_Serializer_writeValue(void* serializerPtr, const char* value) {
    if (!serializerPtr || !value) return;
    NovaSerializer* s = (NovaSerializer*)serializerPtr;

    size_t len = strlen(value);
    // Write string tag
    s->buffer.push_back(0x22);  // String tag
    // Write length (simplified varint)
    s->buffer.push_back((uint8_t)(len & 0xFF));
    if (len > 127) {
        s->buffer.push_back((uint8_t)((len >> 8) & 0xFF));
    }
    // Write string bytes
    for (size_t i = 0; i < len; i++) {
        s->buffer.push_back((uint8_t)value[i]);
    }
}

// Write uint32
void nova_v8_Serializer_writeUint32(void* serializerPtr, uint32_t value) {
    if (!serializerPtr) return;
    NovaSerializer* s = (NovaSerializer*)serializerPtr;

    s->buffer.push_back(0x49);  // Int tag
    s->buffer.push_back((uint8_t)(value & 0xFF));
    s->buffer.push_back((uint8_t)((value >> 8) & 0xFF));
    s->buffer.push_back((uint8_t)((value >> 16) & 0xFF));
    s->buffer.push_back((uint8_t)((value >> 24) & 0xFF));
}

// Write uint64
void nova_v8_Serializer_writeUint64(void* serializerPtr, uint64_t value) {
    if (!serializerPtr) return;
    NovaSerializer* s = (NovaSerializer*)serializerPtr;

    s->buffer.push_back(0x4E);  // BigInt tag (simplified)
    for (int i = 0; i < 8; i++) {
        s->buffer.push_back((uint8_t)((value >> (i * 8)) & 0xFF));
    }
}

// Write double
void nova_v8_Serializer_writeDouble(void* serializerPtr, double value) {
    if (!serializerPtr) return;
    NovaSerializer* s = (NovaSerializer*)serializerPtr;

    s->buffer.push_back(0x4E);  // Number tag
    uint8_t* bytes = (uint8_t*)&value;
    for (int i = 0; i < 8; i++) {
        s->buffer.push_back(bytes[i]);
    }
}

// Write raw bytes
void nova_v8_Serializer_writeRawBytes(void* serializerPtr, const uint8_t* data, int length) {
    if (!serializerPtr || !data || length <= 0) return;
    NovaSerializer* s = (NovaSerializer*)serializerPtr;

    for (int i = 0; i < length; i++) {
        s->buffer.push_back(data[i]);
    }
}

// Release buffer - returns buffer and transfers ownership
const uint8_t* nova_v8_Serializer_releaseBuffer(void* serializerPtr, int* length) {
    if (!serializerPtr || !length) return nullptr;
    NovaSerializer* s = (NovaSerializer*)serializerPtr;

    *length = (int)s->buffer.size();
    // Copy to persistent buffer
    static thread_local std::vector<uint8_t> result;
    result = s->buffer;
    return result.data();
}

// Free serializer
void nova_v8_Serializer_free(void* serializerPtr) {
    if (serializerPtr) delete (NovaSerializer*)serializerPtr;
}

// ============================================================================
// Deserializer
// ============================================================================

struct NovaDeserializer {
    const uint8_t* buffer;
    size_t length;
    size_t offset;
};

// Create deserializer
void* nova_v8_Deserializer_create(const uint8_t* buffer, int length) {
    if (!buffer || length <= 0) return nullptr;

    NovaDeserializer* d = new NovaDeserializer();
    d->buffer = buffer;
    d->length = (size_t)length;
    d->offset = 0;
    return d;
}

// Read header
int nova_v8_Deserializer_readHeader(void* deserializerPtr) {
    if (!deserializerPtr) return 0;
    NovaDeserializer* d = (NovaDeserializer*)deserializerPtr;

    if (d->offset + 2 > d->length) return 0;
    if (d->buffer[d->offset] != 0xFF) return 0;
    d->offset += 2;
    return 1;
}

// Read value (as string)
const char* nova_v8_Deserializer_readValue(void* deserializerPtr) {
    if (!deserializerPtr) return nullptr;
    NovaDeserializer* d = (NovaDeserializer*)deserializerPtr;

    static thread_local std::string result;
    result.clear();

    if (d->offset >= d->length) return nullptr;

    uint8_t tag = d->buffer[d->offset++];
    if (tag == 0x22) {  // String
        if (d->offset >= d->length) return nullptr;
        size_t len = d->buffer[d->offset++];
        if (len > 127 && d->offset < d->length) {
            len |= ((size_t)d->buffer[d->offset++] << 8);
        }
        if (d->offset + len > d->length) return nullptr;
        result.assign((const char*)(d->buffer + d->offset), len);
        d->offset += len;
    }

    return result.c_str();
}

// Read uint32
uint32_t nova_v8_Deserializer_readUint32(void* deserializerPtr) {
    if (!deserializerPtr) return 0;
    NovaDeserializer* d = (NovaDeserializer*)deserializerPtr;

    if (d->offset + 5 > d->length) return 0;
    d->offset++;  // Skip tag
    uint32_t value = 0;
    for (int i = 0; i < 4; i++) {
        value |= ((uint32_t)d->buffer[d->offset++] << (i * 8));
    }
    return value;
}

// Read uint64
uint64_t nova_v8_Deserializer_readUint64(void* deserializerPtr) {
    if (!deserializerPtr) return 0;
    NovaDeserializer* d = (NovaDeserializer*)deserializerPtr;

    if (d->offset + 9 > d->length) return 0;
    d->offset++;  // Skip tag
    uint64_t value = 0;
    for (int i = 0; i < 8; i++) {
        value |= ((uint64_t)d->buffer[d->offset++] << (i * 8));
    }
    return value;
}

// Read double
double nova_v8_Deserializer_readDouble(void* deserializerPtr) {
    if (!deserializerPtr) return 0.0;
    NovaDeserializer* d = (NovaDeserializer*)deserializerPtr;

    if (d->offset + 9 > d->length) return 0.0;
    d->offset++;  // Skip tag
    double value;
    uint8_t* bytes = (uint8_t*)&value;
    for (int i = 0; i < 8; i++) {
        bytes[i] = d->buffer[d->offset++];
    }
    return value;
}

// Read raw bytes
int nova_v8_Deserializer_readRawBytes(void* deserializerPtr, uint8_t* out, int length) {
    if (!deserializerPtr || !out || length <= 0) return 0;
    NovaDeserializer* d = (NovaDeserializer*)deserializerPtr;

    int available = (int)(d->length - d->offset);
    int toRead = (length < available) ? length : available;
    memcpy(out, d->buffer + d->offset, toRead);
    d->offset += toRead;
    return toRead;
}

// Get wire format version
int nova_v8_Deserializer_getWireFormatVersion(void* deserializerPtr) {
    (void)deserializerPtr;
    return 15;  // V8 wire format version
}

// Free deserializer
void nova_v8_Deserializer_free(void* deserializerPtr) {
    if (deserializerPtr) delete (NovaDeserializer*)deserializerPtr;
}

// ============================================================================
// Convenience serialize/deserialize
// ============================================================================

const uint8_t* nova_v8_serialize(const char* value, int* length) {
    void* s = nova_v8_Serializer_create();
    nova_v8_Serializer_writeHeader(s);
    nova_v8_Serializer_writeValue(s, value);
    const uint8_t* result = nova_v8_Serializer_releaseBuffer(s, length);
    nova_v8_Serializer_free(s);
    return result;
}

const char* nova_v8_deserialize(const uint8_t* buffer, int length) {
    void* d = nova_v8_Deserializer_create(buffer, length);
    if (!d) return nullptr;
    if (!nova_v8_Deserializer_readHeader(d)) {
        nova_v8_Deserializer_free(d);
        return nullptr;
    }
    const char* result = nova_v8_Deserializer_readValue(d);
    nova_v8_Deserializer_free(d);
    return result;
}

// ============================================================================
// V8 Flags
// ============================================================================

static std::string v8Flags;

void nova_v8_setFlagsFromString(const char* flags) {
    if (flags) {
        v8Flags = flags;
        // Note: Nova doesn't use V8, so flags are stored but not applied
    }
}

const char* nova_v8_getFlagsAsString() {
    return v8Flags.c_str();
}

// ============================================================================
// Coverage
// ============================================================================

static bool coverageEnabled = false;

void nova_v8_takeCoverage() {
    coverageEnabled = true;
    // In a real implementation, this would trigger coverage collection
}

void nova_v8_stopCoverage() {
    coverageEnabled = false;
}

int nova_v8_isCoverageEnabled() {
    return coverageEnabled ? 1 : 0;
}

// ============================================================================
// GC Control (calls Nova's mark-and-sweep collector)
// ============================================================================

void nova_v8_gc() {
    // Trigger Nova's garbage collector
    nova::runtime::collect_garbage();
}

void nova_v8_gcMinor() {
    // Nova doesn't distinguish minor/major GC - run full collection
    nova::runtime::collect_garbage();
}

void nova_v8_gcMajor() {
    // Run full garbage collection
    nova::runtime::collect_garbage();
}

// ============================================================================
// Promise Hooks (experimental)
// ============================================================================

typedef void (*PromiseHookCallback)(int type, void* promise, void* parent);
static PromiseHookCallback promiseHook = nullptr;

void nova_v8_promiseHooks_onInit(PromiseHookCallback callback) {
    promiseHook = callback;
}

void nova_v8_promiseHooks_onSettled(PromiseHookCallback callback) {
    (void)callback;
    // Store settled callback
}

void nova_v8_promiseHooks_onBefore(PromiseHookCallback callback) {
    (void)callback;
    // Store before callback
}

void nova_v8_promiseHooks_onAfter(PromiseHookCallback callback) {
    (void)callback;
    // Store after callback
}

void* nova_v8_promiseHooks_createHook(PromiseHookCallback init, PromiseHookCallback before,
                                       PromiseHookCallback after, PromiseHookCallback settled) {
    // Create and return a hook object
    (void)init; (void)before; (void)after; (void)settled;
    return (void*)1;  // Dummy handle
}

void nova_v8_promiseHooks_enable(void* hook) {
    (void)hook;
}

void nova_v8_promiseHooks_disable(void* hook) {
    (void)hook;
}

// ============================================================================
// Startup Snapshot (experimental)
// ============================================================================

int nova_v8_startupSnapshot_isBuildingSnapshot() {
    return 0;  // Nova doesn't use startup snapshots
}

void nova_v8_startupSnapshot_addSerializeCallback(void* callback, void* data) {
    (void)callback; (void)data;
}

void nova_v8_startupSnapshot_addDeserializeCallback(void* callback, void* data) {
    (void)callback; (void)data;
}

void nova_v8_startupSnapshot_setDeserializeMainFunction(void* callback, void* data) {
    (void)callback; (void)data;
}

// ============================================================================
// Query Objects
// ============================================================================

const char* nova_v8_queryObjects(const char* constructorName) {
    static thread_local std::string result;
    // Return empty array - Nova doesn't track objects by constructor
    (void)constructorName;
    result = "[]";
    return result.c_str();
}

// ============================================================================
// Memory Pressure
// ============================================================================

void nova_v8_setMemoryPressure(int level) {
    // 0 = none, 1 = moderate, 2 = critical
    (void)level;
    // Could trigger GC based on pressure level
}

// ============================================================================
// Default Deserializer/Serializer Delegates
// ============================================================================

void nova_v8_DefaultSerializer_writeHostObject(void* serializer, void* object) {
    (void)serializer; (void)object;
}

void* nova_v8_DefaultDeserializer_readHostObject(void* deserializer) {
    (void)deserializer;
    return nullptr;
}

} // extern "C"
