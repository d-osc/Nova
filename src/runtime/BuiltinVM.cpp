// Nova Builtin VM Module Implementation
// Provides Node.js-compatible vm API for code execution contexts

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#endif

extern "C" {

// ============================================================================
// Context Structure
// ============================================================================

struct NovaVMContext {
    int64_t id;
    std::unordered_map<std::string, std::string>* globals;
    std::string name;
    std::string origin;
    bool isContext;
    int64_t timeout;
    bool breakOnSigint;
};

static std::atomic<int64_t> nextContextId{1};
static std::unordered_map<int64_t, NovaVMContext*>* contexts = nullptr;
static std::mutex contextMutex;

static void ensureContexts() {
    if (!contexts) {
        contexts = new std::unordered_map<int64_t, NovaVMContext*>();
    }
}

// ============================================================================
// vm.createContext([contextObject[, options]])
// ============================================================================

void* nova_vm_createContext() {
    std::lock_guard<std::mutex> lock(contextMutex);
    ensureContexts();

    NovaVMContext* ctx = new NovaVMContext();
    ctx->id = nextContextId++;
    ctx->globals = new std::unordered_map<std::string, std::string>();
    ctx->name = "VM Context";
    ctx->origin = "";
    ctx->isContext = true;
    ctx->timeout = 0;
    ctx->breakOnSigint = false;

    (*contexts)[ctx->id] = ctx;
    return ctx;
}

void* nova_vm_createContextWithOptions(const char* name, const char* origin,
                                        int64_t timeout, int breakOnSigint) {
    std::lock_guard<std::mutex> lock(contextMutex);
    ensureContexts();

    NovaVMContext* ctx = new NovaVMContext();
    ctx->id = nextContextId++;
    ctx->globals = new std::unordered_map<std::string, std::string>();
    ctx->name = name ? name : "VM Context";
    ctx->origin = origin ? origin : "";
    ctx->isContext = true;
    ctx->timeout = timeout;
    ctx->breakOnSigint = breakOnSigint != 0;

    (*contexts)[ctx->id] = ctx;
    return ctx;
}

// ============================================================================
// vm.isContext(object)
// ============================================================================

int nova_vm_isContext(void* contextPtr) {
    if (!contextPtr) return 0;
    NovaVMContext* ctx = (NovaVMContext*)contextPtr;
    return ctx->isContext ? 1 : 0;
}

// ============================================================================
// Context Global Variables
// ============================================================================

void nova_vm_contextSetGlobal(void* contextPtr, const char* name, const char* value) {
    if (!contextPtr || !name) return;
    NovaVMContext* ctx = (NovaVMContext*)contextPtr;
    (*ctx->globals)[name] = value ? value : "";
}

const char* nova_vm_contextGetGlobal(void* contextPtr, const char* name) {
    if (!contextPtr || !name) return nullptr;
    NovaVMContext* ctx = (NovaVMContext*)contextPtr;

    auto it = ctx->globals->find(name);
    if (it != ctx->globals->end()) {
        return it->second.c_str();
    }
    return nullptr;
}

int nova_vm_contextHasGlobal(void* contextPtr, const char* name) {
    if (!contextPtr || !name) return 0;
    NovaVMContext* ctx = (NovaVMContext*)contextPtr;
    return ctx->globals->find(name) != ctx->globals->end() ? 1 : 0;
}

void nova_vm_contextDeleteGlobal(void* contextPtr, const char* name) {
    if (!contextPtr || !name) return;
    NovaVMContext* ctx = (NovaVMContext*)contextPtr;
    ctx->globals->erase(name);
}

const char* nova_vm_contextGetGlobalNames(void* contextPtr) {
    if (!contextPtr) return "[]";

    NovaVMContext* ctx = (NovaVMContext*)contextPtr;
    static thread_local std::string result;

    std::ostringstream json;
    json << "[";
    bool first = true;
    for (const auto& pair : *ctx->globals) {
        if (!first) json << ",";
        json << "\"" << pair.first << "\"";
        first = false;
    }
    json << "]";

    result = json.str();
    return result.c_str();
}

// ============================================================================
// Script Structure
// ============================================================================

struct NovaVMScript {
    int64_t id;
    std::string code;
    std::string filename;
    int lineOffset;
    int columnOffset;
    std::string cachedData;
    bool produceCachedData;
    bool cachedDataRejected;
    std::string sourceMapURL;
    int64_t timeout;
};

static std::atomic<int64_t> nextScriptId{1};

// ============================================================================
// vm.Script class
// ============================================================================

void* nova_vm_Script_create(const char* code) {
    NovaVMScript* script = new NovaVMScript();
    script->id = nextScriptId++;
    script->code = code ? code : "";
    script->filename = "evalmachine.<anonymous>";
    script->lineOffset = 0;
    script->columnOffset = 0;
    script->produceCachedData = false;
    script->cachedDataRejected = false;
    script->timeout = 0;
    return script;
}

void* nova_vm_Script_createWithOptions(const char* code, const char* filename,
                                        int lineOffset, int columnOffset,
                                        const char* cachedData, int produceCachedData) {
    NovaVMScript* script = new NovaVMScript();
    script->id = nextScriptId++;
    script->code = code ? code : "";
    script->filename = filename ? filename : "evalmachine.<anonymous>";
    script->lineOffset = lineOffset;
    script->columnOffset = columnOffset;
    script->cachedData = cachedData ? cachedData : "";
    script->produceCachedData = produceCachedData != 0;
    script->cachedDataRejected = false;
    script->timeout = 0;

    // If cached data was provided, validate it (simplified)
    if (cachedData && strlen(cachedData) > 0) {
        // In real implementation, validate cached data matches code
        script->cachedDataRejected = false;
    }

    return script;
}

// Get script properties
const char* nova_vm_Script_getCode(void* scriptPtr) {
    if (!scriptPtr) return "";
    return ((NovaVMScript*)scriptPtr)->code.c_str();
}

const char* nova_vm_Script_getFilename(void* scriptPtr) {
    if (!scriptPtr) return "";
    return ((NovaVMScript*)scriptPtr)->filename.c_str();
}

int nova_vm_Script_getLineOffset(void* scriptPtr) {
    if (!scriptPtr) return 0;
    return ((NovaVMScript*)scriptPtr)->lineOffset;
}

int nova_vm_Script_getColumnOffset(void* scriptPtr) {
    if (!scriptPtr) return 0;
    return ((NovaVMScript*)scriptPtr)->columnOffset;
}

int nova_vm_Script_cachedDataRejected(void* scriptPtr) {
    if (!scriptPtr) return 0;
    return ((NovaVMScript*)scriptPtr)->cachedDataRejected ? 1 : 0;
}

const char* nova_vm_Script_sourceMapURL(void* scriptPtr) {
    if (!scriptPtr) return "";
    return ((NovaVMScript*)scriptPtr)->sourceMapURL.c_str();
}

// script.createCachedData()
const char* nova_vm_Script_createCachedData(void* scriptPtr) {
    if (!scriptPtr) return "";

    NovaVMScript* script = (NovaVMScript*)scriptPtr;
    static thread_local std::string result;

    // Create a simple cached data format (in real impl, this would be bytecode)
    std::ostringstream cache;
    cache << "NOVA_CACHE_V1:";
    cache << script->code.length() << ":";
    // Simple checksum
    uint32_t checksum = 0;
    for (char c : script->code) {
        checksum = (checksum * 31) + (uint8_t)c;
    }
    cache << checksum;

    result = cache.str();
    return result.c_str();
}

// ============================================================================
// Script Execution
// ============================================================================

// Execution result callback type
typedef void (*VMExecutionCallback)(const char* result, const char* error);

// script.runInContext(contextifiedObject[, options])
const char* nova_vm_Script_runInContext(void* scriptPtr, void* contextPtr,
                                         int64_t timeout, int breakOnSigint) {
    static thread_local std::string result;
    result.clear();

    if (!scriptPtr) {
        result = "Error: Script is null";
        return result.c_str();
    }

    NovaVMScript* script = (NovaVMScript*)scriptPtr;
    (void)contextPtr;  // Context would be used for variable resolution
    (void)timeout;
    (void)breakOnSigint;

    // In a real implementation, this would:
    // 1. Parse the code
    // 2. Execute in the context
    // 3. Return the result

    // For now, return a placeholder indicating code was "executed"
    result = "[Executed: " + std::to_string(script->code.length()) + " chars]";
    return result.c_str();
}

// script.runInNewContext([contextObject[, options]])
const char* nova_vm_Script_runInNewContext(void* scriptPtr, int64_t timeout) {
    void* ctx = nova_vm_createContext();
    const char* result = nova_vm_Script_runInContext(scriptPtr, ctx, timeout, 0);
    // Note: Context should be cleaned up
    return result;
}

// script.runInThisContext([options])
const char* nova_vm_Script_runInThisContext(void* scriptPtr, int64_t timeout) {
    return nova_vm_Script_runInContext(scriptPtr, nullptr, timeout, 0);
}

// Free script
void nova_vm_Script_free(void* scriptPtr) {
    if (scriptPtr) delete (NovaVMScript*)scriptPtr;
}

// ============================================================================
// Convenience run functions
// ============================================================================

// vm.runInContext(code, contextifiedObject[, options])
const char* nova_vm_runInContext(const char* code, void* contextPtr,
                                  const char* filename, int64_t timeout) {
    void* script = nova_vm_Script_createWithOptions(code, filename, 0, 0, nullptr, 0);
    const char* result = nova_vm_Script_runInContext(script, contextPtr, timeout, 0);
    nova_vm_Script_free(script);
    return result;
}

// vm.runInNewContext(code[, contextObject[, options]])
const char* nova_vm_runInNewContext(const char* code, const char* filename, int64_t timeout) {
    void* script = nova_vm_Script_createWithOptions(code, filename, 0, 0, nullptr, 0);
    const char* result = nova_vm_Script_runInNewContext(script, timeout);
    nova_vm_Script_free(script);
    return result;
}

// vm.runInThisContext(code[, options])
const char* nova_vm_runInThisContext(const char* code, const char* filename, int64_t timeout) {
    void* script = nova_vm_Script_createWithOptions(code, filename, 0, 0, nullptr, 0);
    const char* result = nova_vm_Script_runInThisContext(script, timeout);
    nova_vm_Script_free(script);
    return result;
}

// ============================================================================
// vm.compileFunction(code[, params[, options]])
// ============================================================================

struct NovaVMCompiledFunction {
    int64_t id;
    std::string code;
    std::vector<std::string> params;
    std::string filename;
    void* contextPtr;
    std::string cachedData;
};

static std::atomic<int64_t> nextFunctionId{1};

void* nova_vm_compileFunction(const char* code, const char** params, int paramCount,
                               const char* filename, void* contextPtr) {
    NovaVMCompiledFunction* fn = new NovaVMCompiledFunction();
    fn->id = nextFunctionId++;
    fn->code = code ? code : "";
    fn->filename = filename ? filename : "evalmachine.<anonymous>";
    fn->contextPtr = contextPtr;

    if (params && paramCount > 0) {
        for (int i = 0; i < paramCount; i++) {
            if (params[i]) {
                fn->params.push_back(params[i]);
            }
        }
    }

    return fn;
}

const char* nova_vm_compiledFunction_getCode(void* fnPtr) {
    if (!fnPtr) return "";
    return ((NovaVMCompiledFunction*)fnPtr)->code.c_str();
}

const char* nova_vm_compiledFunction_getParams(void* fnPtr) {
    if (!fnPtr) return "[]";

    NovaVMCompiledFunction* fn = (NovaVMCompiledFunction*)fnPtr;
    static thread_local std::string result;

    std::ostringstream json;
    json << "[";
    for (size_t i = 0; i < fn->params.size(); i++) {
        if (i > 0) json << ",";
        json << "\"" << fn->params[i] << "\"";
    }
    json << "]";

    result = json.str();
    return result.c_str();
}

const char* nova_vm_compiledFunction_createCachedData(void* fnPtr) {
    if (!fnPtr) return "";
    NovaVMCompiledFunction* fn = (NovaVMCompiledFunction*)fnPtr;

    static thread_local std::string result;
    std::ostringstream cache;
    cache << "NOVA_FN_CACHE_V1:" << fn->code.length() << ":" << fn->params.size();
    result = cache.str();
    return result.c_str();
}

void nova_vm_compiledFunction_free(void* fnPtr) {
    if (fnPtr) delete (NovaVMCompiledFunction*)fnPtr;
}

// ============================================================================
// vm.Module (experimental)
// ============================================================================

enum ModuleStatus {
    MODULE_UNLINKED = 0,
    MODULE_LINKING = 1,
    MODULE_LINKED = 2,
    MODULE_EVALUATING = 3,
    MODULE_EVALUATED = 4,
    MODULE_ERRORED = 5
};

struct NovaVMModule {
    int64_t id;
    std::string identifier;
    std::string code;
    ModuleStatus status;
    std::string error;
    std::vector<std::string> dependencySpecifiers;
    void* namespace_obj;
    void* context;
};

static std::atomic<int64_t> nextModuleId{1};

// vm.SourceTextModule
void* nova_vm_SourceTextModule_create(const char* code, const char* identifier,
                                       void* context) {
    NovaVMModule* mod = new NovaVMModule();
    mod->id = nextModuleId++;
    mod->code = code ? code : "";
    mod->identifier = identifier ? identifier : "vm:module";
    mod->status = MODULE_UNLINKED;
    mod->namespace_obj = nullptr;
    mod->context = context;

    // Parse import statements to find dependencies (simplified)
    // In real impl, this would parse the code properly
    return mod;
}

// Module properties
const char* nova_vm_Module_identifier(void* modulePtr) {
    if (!modulePtr) return "";
    return ((NovaVMModule*)modulePtr)->identifier.c_str();
}

int nova_vm_Module_status(void* modulePtr) {
    if (!modulePtr) return MODULE_ERRORED;
    return (int)((NovaVMModule*)modulePtr)->status;
}

const char* nova_vm_Module_statusString(void* modulePtr) {
    if (!modulePtr) return "errored";
    switch (((NovaVMModule*)modulePtr)->status) {
        case MODULE_UNLINKED: return "unlinked";
        case MODULE_LINKING: return "linking";
        case MODULE_LINKED: return "linked";
        case MODULE_EVALUATING: return "evaluating";
        case MODULE_EVALUATED: return "evaluated";
        case MODULE_ERRORED: return "errored";
        default: return "unknown";
    }
}

const char* nova_vm_Module_error(void* modulePtr) {
    if (!modulePtr) return "";
    return ((NovaVMModule*)modulePtr)->error.c_str();
}

const char* nova_vm_Module_dependencySpecifiers(void* modulePtr) {
    if (!modulePtr) return "[]";

    NovaVMModule* mod = (NovaVMModule*)modulePtr;
    static thread_local std::string result;

    std::ostringstream json;
    json << "[";
    for (size_t i = 0; i < mod->dependencySpecifiers.size(); i++) {
        if (i > 0) json << ",";
        json << "\"" << mod->dependencySpecifiers[i] << "\"";
    }
    json << "]";

    result = json.str();
    return result.c_str();
}

void* nova_vm_Module_namespace(void* modulePtr) {
    if (!modulePtr) return nullptr;
    return ((NovaVMModule*)modulePtr)->namespace_obj;
}

// module.link(linker)
typedef void* (*ModuleLinker)(const char* specifier, void* referencingModule);

int nova_vm_Module_link(void* modulePtr, ModuleLinker linker) {
    if (!modulePtr) return -1;
    NovaVMModule* mod = (NovaVMModule*)modulePtr;

    mod->status = MODULE_LINKING;

    // Link all dependencies
    for (const auto& specifier : mod->dependencySpecifiers) {
        void* dep = linker(specifier.c_str(), modulePtr);
        if (!dep) {
            mod->status = MODULE_ERRORED;
            mod->error = "Cannot resolve module: " + specifier;
            return -1;
        }
    }

    mod->status = MODULE_LINKED;
    return 0;
}

// module.evaluate([options])
const char* nova_vm_Module_evaluate(void* modulePtr, int64_t timeout) {
    static thread_local std::string result;

    if (!modulePtr) {
        result = "Error: Module is null";
        return result.c_str();
    }

    NovaVMModule* mod = (NovaVMModule*)modulePtr;
    (void)timeout;

    if (mod->status != MODULE_LINKED) {
        result = "Error: Module must be linked before evaluation";
        return result.c_str();
    }

    mod->status = MODULE_EVALUATING;

    // In real implementation, evaluate the module
    mod->status = MODULE_EVALUATED;
    result = "[Module evaluated]";
    return result.c_str();
}

void nova_vm_Module_free(void* modulePtr) {
    if (modulePtr) delete (NovaVMModule*)modulePtr;
}

// ============================================================================
// vm.SyntheticModule
// ============================================================================

void* nova_vm_SyntheticModule_create(const char** exportNames, int exportCount,
                                      const char* identifier, void* context) {
    NovaVMModule* mod = new NovaVMModule();
    mod->id = nextModuleId++;
    mod->identifier = identifier ? identifier : "vm:synthetic";
    mod->status = MODULE_UNLINKED;
    mod->namespace_obj = nullptr;
    mod->context = context;

    // Store export names as dependency specifiers (repurposed)
    if (exportNames && exportCount > 0) {
        for (int i = 0; i < exportCount; i++) {
            if (exportNames[i]) {
                mod->dependencySpecifiers.push_back(exportNames[i]);
            }
        }
    }

    return mod;
}

void nova_vm_SyntheticModule_setExport(void* modulePtr, const char* name, const char* value) {
    if (!modulePtr || !name) return;
    // In real impl, store export value
    (void)value;
}

// ============================================================================
// vm.measureMemory([options])
// ============================================================================

const char* nova_vm_measureMemory(int detailed) {
    static thread_local std::string result;

    // Get memory info (simplified)
    size_t total = 0, jsMemory = 0;

#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        total = pmc.WorkingSetSize;
        jsMemory = pmc.PrivateUsage;
    }
#else
    // Use /proc/self/statm on Linux
    FILE* f = fopen("/proc/self/statm", "r");
    if (f) {
        unsigned long size, resident;
        if (fscanf(f, "%lu %lu", &size, &resident) == 2) {
            total = resident * 4096;  // Page size
            jsMemory = total;
        }
        fclose(f);
    }
#endif

    std::ostringstream json;
    if (detailed) {
        json << "{\"total\":{\"jsMemoryEstimate\":" << jsMemory
             << ",\"jsMemoryRange\":[" << (jsMemory / 2) << "," << (jsMemory * 2) << "]},"
             << "\"current\":{\"jsMemoryEstimate\":" << jsMemory
             << ",\"jsMemoryRange\":[" << (jsMemory / 2) << "," << (jsMemory * 2) << "]},"
             << "\"other\":[]}";
    } else {
        json << "{\"total\":{\"jsMemoryEstimate\":" << jsMemory
             << ",\"jsMemoryRange\":[" << (jsMemory / 2) << "," << (jsMemory * 2) << "]}}";
    }

    result = json.str();
    return result.c_str();
}

// ============================================================================
// Context cleanup
// ============================================================================

void nova_vm_contextFree(void* contextPtr) {
    if (!contextPtr) return;

    std::lock_guard<std::mutex> lock(contextMutex);
    NovaVMContext* ctx = (NovaVMContext*)contextPtr;

    if (contexts) {
        contexts->erase(ctx->id);
    }

    delete ctx->globals;
    delete ctx;
}

void nova_vm_cleanup() {
    std::lock_guard<std::mutex> lock(contextMutex);
    if (contexts) {
        for (auto& pair : *contexts) {
            delete pair.second->globals;
            delete pair.second;
        }
        delete contexts;
        contexts = nullptr;
    }
}

// ============================================================================
// Constants
// ============================================================================

void* nova_vm_constants_USE_MAIN_CONTEXT_DEFAULT_LOADER() {
    static int marker = 1;
    return &marker;
}

// ============================================================================
// Microtask Queue
// ============================================================================

static std::vector<void*>* microtaskQueue = nullptr;

void nova_vm_queueMicrotask(void* callback) {
    if (!microtaskQueue) {
        microtaskQueue = new std::vector<void*>();
    }
    microtaskQueue->push_back(callback);
}

void nova_vm_runMicrotasks() {
    if (!microtaskQueue || microtaskQueue->empty()) return;

    typedef void (*MicrotaskCallback)();
    std::vector<void*> tasks = *microtaskQueue;
    microtaskQueue->clear();

    for (void* task : tasks) {
        if (task) {
            ((MicrotaskCallback)task)();
        }
    }
}

int nova_vm_hasPendingMicrotasks() {
    return (microtaskQueue && !microtaskQueue->empty()) ? 1 : 0;
}

} // extern "C"
