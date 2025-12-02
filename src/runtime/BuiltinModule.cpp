// Nova Module System - Node.js compatible CommonJS require/module
// Provides require(), module, exports functionality

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"
#else
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"
#endif

// ============================================================================
// Module Cache (outside extern "C")
// ============================================================================

struct NovaModule {
    char* id;
    char* filename;
    char* path;
    void* exports;
    void* parent;
    bool loaded;
    std::vector<NovaModule*> children;
    std::vector<std::string> paths;
};

static std::map<std::string, NovaModule*> moduleCache;
static NovaModule* mainModule = nullptr;
static std::vector<std::string> builtinModules = {
    "assert", "async_hooks", "buffer", "child_process", "cluster",
    "console", "constants", "crypto", "dgram", "diagnostics_channel",
    "dns", "domain", "events", "fs", "http", "http2", "https",
    "inspector", "module", "net", "os", "path", "perf_hooks",
    "process", "punycode", "querystring", "readline", "repl",
    "stream", "string_decoder", "timers", "tls", "trace_events",
    "tty", "url", "util", "v8", "vm", "wasi", "worker_threads", "zlib"
};

static std::map<std::string, void*> extensions;

// Helper to allocate string
static char* allocString(const std::string& s) {
    char* result = (char*)malloc(s.size() + 1);
    if (result) {
        memcpy(result, s.c_str(), s.size() + 1);
    }
    return result;
}

// ============================================================================
// Path utilities (outside extern "C")
// ============================================================================

static std::string normalizePath(const std::string& path) {
    std::string result = path;
    // Convert backslashes to forward slashes
    for (char& c : result) {
        if (c == '\\') c = '/';
    }
    // Remove trailing slash
    while (!result.empty() && result.back() == '/') {
        result.pop_back();
    }
    return result;
}

static std::string dirname(const std::string& path) {
    std::string normalized = normalizePath(path);
    size_t pos = normalized.rfind('/');
    if (pos == std::string::npos) return ".";
    if (pos == 0) return "/";
    return normalized.substr(0, pos);
}

static std::string basename(const std::string& path) {
    std::string normalized = normalizePath(path);
    size_t pos = normalized.rfind('/');
    if (pos == std::string::npos) return normalized;
    return normalized.substr(pos + 1);
}

static std::string joinPath(const std::string& dir, const std::string& file) {
    if (dir.empty()) return file;
    if (file.empty()) return dir;
    std::string d = normalizePath(dir);
    std::string f = normalizePath(file);
    if (f[0] == '/') return f;
    return d + "/" + f;
}

static bool fileExists(const std::string& path) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode));
#endif
}

static bool dirExists(const std::string& path) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
#endif
}

// ============================================================================
// Module resolution
// ============================================================================

static std::vector<std::string> nodeModulePaths(const std::string& from) {
    std::vector<std::string> paths;
    std::string dir = normalizePath(from);

    while (!dir.empty() && dir != "/") {
        if (basename(dir) != "node_modules") {
            paths.push_back(joinPath(dir, "node_modules"));
        }
        dir = dirname(dir);
    }

    // Add global paths
#ifdef _WIN32
    char* appdata = getenv("APPDATA");
    if (appdata) {
        paths.push_back(std::string(appdata) + "\\npm\\node_modules");
    }
#else
    paths.push_back("/usr/local/lib/node_modules");
    paths.push_back("/usr/lib/node_modules");
    char* home = getenv("HOME");
    if (home) {
        paths.push_back(std::string(home) + "/.node_modules");
        paths.push_back(std::string(home) + "/.node_libraries");
    }
#endif

    return paths;
}

static std::string tryFile(const std::string& path) {
    if (fileExists(path)) return path;
    return "";
}

static std::string tryExtensions(const std::string& path) {
    static const char* extensions[] = { ".js", ".json", ".node", ".ts", ".mjs", ".cjs" };

    std::string result = tryFile(path);
    if (!result.empty()) return result;

    for (const char* ext : extensions) {
        result = tryFile(path + ext);
        if (!result.empty()) return result;
    }

    return "";
}

static std::string tryPackage(const std::string& dir) {
    std::string pkgPath = joinPath(dir, "package.json");
    if (!fileExists(pkgPath)) return "";

    // Simplified: assume main is "index.js"
    std::string mainPath = joinPath(dir, "index.js");
    if (fileExists(mainPath)) return mainPath;

    return tryExtensions(joinPath(dir, "index"));
}

static std::string resolveFilename(const std::string& request, const std::string& parent) {
    // Check if builtin
    for (const std::string& builtin : builtinModules) {
        if (request == builtin || request == "node:" + builtin) {
            return "node:" + builtin;
        }
    }

    std::string parentDir = parent.empty() ? "." : dirname(parent);

    // Relative path
    if (request[0] == '.' || request[0] == '/') {
        std::string absPath = joinPath(parentDir, request);

        // Try as file
        std::string result = tryExtensions(absPath);
        if (!result.empty()) return result;

        // Try as directory
        if (dirExists(absPath)) {
            result = tryPackage(absPath);
            if (!result.empty()) return result;
        }

        return "";
    }

    // Node modules
    std::vector<std::string> paths = nodeModulePaths(parentDir);
    for (const std::string& nodePath : paths) {
        std::string modulePath = joinPath(nodePath, request);

        // Try as file
        std::string result = tryExtensions(modulePath);
        if (!result.empty()) return result;

        // Try as directory
        if (dirExists(modulePath)) {
            result = tryPackage(modulePath);
            if (!result.empty()) return result;
        }
    }

    return "";
}

// ============================================================================
// C API starts here
// ============================================================================

extern "C" {

// ============================================================================
// require() function
// ============================================================================

void* nova_require(const char* id) {
    if (!id) return nullptr;

    std::string request = id;
    std::string parentPath = mainModule ? mainModule->filename : "";

    std::string filename = resolveFilename(request, parentPath);
    if (filename.empty()) {
        fprintf(stderr, "Error: Cannot find module '%s'\n", id);
        return nullptr;
    }

    // Check cache
    auto it = moduleCache.find(filename);
    if (it != moduleCache.end()) {
        return it->second->exports;
    }

    // Create new module
    NovaModule* mod = new NovaModule();
    mod->id = allocString(filename);
    mod->filename = allocString(filename);
    mod->path = allocString(dirname(filename));
    mod->exports = nullptr;
    mod->parent = mainModule;
    mod->loaded = false;

    // Add to cache before loading (handles circular deps)
    moduleCache[filename] = mod;

    // For builtin modules, exports would be populated by the runtime
    if (filename.find("node:") == 0) {
        mod->loaded = true;
        // Builtin module exports handled elsewhere
    }

    mod->loaded = true;
    return mod->exports;
}

void* nova_require_from(const char* id, const char* parentFilename) {
    if (!id) return nullptr;

    std::string request = id;
    std::string parentPath = parentFilename ? parentFilename : "";

    std::string filename = resolveFilename(request, parentPath);
    if (filename.empty()) {
        fprintf(stderr, "Error: Cannot find module '%s'\n", id);
        return nullptr;
    }

    // Check cache
    auto it = moduleCache.find(filename);
    if (it != moduleCache.end()) {
        return it->second->exports;
    }

    // Create and cache module (simplified)
    NovaModule* mod = new NovaModule();
    mod->id = allocString(filename);
    mod->filename = allocString(filename);
    mod->path = allocString(dirname(filename));
    mod->exports = nullptr;
    mod->parent = nullptr;
    mod->loaded = true;

    moduleCache[filename] = mod;
    return mod->exports;
}

// ============================================================================
// require.resolve()
// ============================================================================

char* nova_require_resolve(const char* request) {
    if (!request) return nullptr;

    std::string parentPath = mainModule ? mainModule->filename : "";
    std::string filename = resolveFilename(request, parentPath);

    if (filename.empty()) return nullptr;
    return allocString(filename);
}

char* nova_require_resolve_from(const char* request, const char* parentFilename) {
    if (!request) return nullptr;

    std::string parentPath = parentFilename ? parentFilename : "";
    std::string filename = resolveFilename(request, parentPath);

    if (filename.empty()) return nullptr;
    return allocString(filename);
}

char** nova_require_resolve_paths(const char* request, int* count) {
    if (!request || !count) return nullptr;

    std::string parentPath = mainModule ? mainModule->filename : ".";
    std::vector<std::string> paths = nodeModulePaths(dirname(parentPath));

    *count = (int)paths.size();
    if (*count == 0) return nullptr;

    char** result = (char**)malloc(sizeof(char*) * (*count));
    for (int i = 0; i < *count; i++) {
        result[i] = allocString(paths[i]);
    }
    return result;
}

// ============================================================================
// require.cache
// ============================================================================

void* nova_require_cache() {
    // Returns the module cache (simplified)
    return &moduleCache;
}

void nova_require_cache_delete(const char* filename) {
    if (!filename) return;

    auto it = moduleCache.find(filename);
    if (it != moduleCache.end()) {
        NovaModule* mod = it->second;
        free(mod->id);
        free(mod->filename);
        free(mod->path);
        delete mod;
        moduleCache.erase(it);
    }
}

int nova_require_cache_has(const char* filename) {
    if (!filename) return 0;
    return moduleCache.find(filename) != moduleCache.end() ? 1 : 0;
}

char** nova_require_cache_keys(int* count) {
    if (!count) return nullptr;

    *count = (int)moduleCache.size();
    if (*count == 0) return nullptr;

    char** result = (char**)malloc(sizeof(char*) * (*count));
    int i = 0;
    for (const auto& pair : moduleCache) {
        result[i++] = allocString(pair.first);
    }
    return result;
}

// ============================================================================
// require.main
// ============================================================================

void* nova_require_main() {
    return mainModule;
}

void nova_require_set_main(const char* filename) {
    if (mainModule) {
        free(mainModule->id);
        free(mainModule->filename);
        free(mainModule->path);
        delete mainModule;
    }

    mainModule = new NovaModule();
    mainModule->id = allocString(filename ? filename : ".");
    mainModule->filename = allocString(filename ? filename : "");
    mainModule->path = allocString(filename ? dirname(filename) : ".");
    mainModule->exports = nullptr;
    mainModule->parent = nullptr;
    mainModule->loaded = true;

    if (filename) {
        moduleCache[filename] = mainModule;
    }
}

// ============================================================================
// Module class
// ============================================================================

void* nova_module_new(const char* id, const char* parent) {
    NovaModule* mod = new NovaModule();
    mod->id = allocString(id ? id : "");
    mod->filename = allocString(id ? id : "");
    mod->path = allocString(id ? dirname(id) : ".");
    mod->exports = nullptr;
    mod->parent = nullptr;
    mod->loaded = false;

    if (parent) {
        auto it = moduleCache.find(parent);
        if (it != moduleCache.end()) {
            mod->parent = it->second;
        }
    }

    return mod;
}

void nova_module_free(void* module) {
    NovaModule* mod = (NovaModule*)module;
    if (mod) {
        free(mod->id);
        free(mod->filename);
        free(mod->path);
        delete mod;
    }
}

char* nova_module_id(void* module) {
    NovaModule* mod = (NovaModule*)module;
    return mod ? allocString(mod->id) : nullptr;
}

char* nova_module_filename(void* module) {
    NovaModule* mod = (NovaModule*)module;
    return mod ? allocString(mod->filename) : nullptr;
}

char* nova_module_path(void* module) {
    NovaModule* mod = (NovaModule*)module;
    return mod ? allocString(mod->path) : nullptr;
}

void* nova_module_exports(void* module) {
    NovaModule* mod = (NovaModule*)module;
    return mod ? mod->exports : nullptr;
}

void nova_module_set_exports(void* module, void* exports) {
    NovaModule* mod = (NovaModule*)module;
    if (mod) mod->exports = exports;
}

void* nova_module_parent(void* module) {
    NovaModule* mod = (NovaModule*)module;
    return mod ? mod->parent : nullptr;
}

int nova_module_loaded(void* module) {
    NovaModule* mod = (NovaModule*)module;
    return (mod && mod->loaded) ? 1 : 0;
}

void nova_module_set_loaded(void* module, int loaded) {
    NovaModule* mod = (NovaModule*)module;
    if (mod) mod->loaded = loaded != 0;
}

char** nova_module_paths(void* module, int* count) {
    NovaModule* mod = (NovaModule*)module;
    if (!mod || !count) return nullptr;

    *count = (int)mod->paths.size();
    if (*count == 0) return nullptr;

    char** result = (char**)malloc(sizeof(char*) * (*count));
    for (int i = 0; i < *count; i++) {
        result[i] = allocString(mod->paths[i]);
    }
    return result;
}

void** nova_module_children(void* module, int* count) {
    NovaModule* mod = (NovaModule*)module;
    if (!mod || !count) return nullptr;

    *count = (int)mod->children.size();
    if (*count == 0) return nullptr;

    void** result = (void**)malloc(sizeof(void*) * (*count));
    for (int i = 0; i < *count; i++) {
        result[i] = mod->children[i];
    }
    return result;
}

// ============================================================================
// Module static methods
// ============================================================================

char** nova_module_builtinModules(int* count) {
    if (!count) return nullptr;

    *count = (int)builtinModules.size();
    char** result = (char**)malloc(sizeof(char*) * (*count));
    for (int i = 0; i < *count; i++) {
        result[i] = allocString(builtinModules[i]);
    }
    return result;
}

int nova_module_isBuiltin(const char* moduleName) {
    if (!moduleName) return 0;

    std::string name = moduleName;
    if (name.find("node:") == 0) {
        name = name.substr(5);
    }

    for (const std::string& builtin : builtinModules) {
        if (name == builtin) return 1;
    }
    return 0;
}

void* nova_module_createRequire(const char* filename) {
    // Returns a require function bound to the given filename
    // In practice, this would return a function object
    // For now, we just store the filename context
    return allocString(filename ? filename : "");
}

char* nova_module_wrap(const char* script) {
    if (!script) return nullptr;

    std::string wrapped = "(function(exports, require, module, __filename, __dirname) { ";
    wrapped += script;
    wrapped += "\n});";

    return allocString(wrapped);
}

char* nova_module_findSourceMap(const char* path) {
    if (!path) return nullptr;

    // Look for .map file
    std::string mapPath = std::string(path) + ".map";
    if (fileExists(mapPath)) {
        return allocString(mapPath);
    }

    return nullptr;
}

void nova_module_syncBuiltinESMExports() {
    // Sync builtin CommonJS exports to ESM namespace
    // Implementation depends on runtime
}

// ============================================================================
// require.extensions (deprecated but still used)
// ============================================================================

void* nova_require_extensions() {
    return &extensions;
}

void nova_require_extensions_set(const char* ext, void* handler) {
    if (ext) {
        extensions[ext] = handler;
    }
}

void* nova_require_extensions_get(const char* ext) {
    if (!ext) return nullptr;

    auto it = extensions.find(ext);
    return (it != extensions.end()) ? it->second : nullptr;
}

// ============================================================================
// Module._nodeModulePaths and related
// ============================================================================

char** nova_module_nodeModulePaths(const char* from, int* count) {
    if (!count) return nullptr;

    std::string fromPath = from ? from : ".";
    std::vector<std::string> paths = nodeModulePaths(fromPath);

    *count = (int)paths.size();
    if (*count == 0) return nullptr;

    char** result = (char**)malloc(sizeof(char*) * (*count));
    for (int i = 0; i < *count; i++) {
        result[i] = allocString(paths[i]);
    }
    return result;
}

char* nova_module_resolveFilename(const char* request, const char* parent) {
    if (!request) return nullptr;

    std::string filename = resolveFilename(request, parent ? parent : "");
    if (filename.empty()) return nullptr;

    return allocString(filename);
}

// ============================================================================
// Module instance methods
// ============================================================================

void* nova_module_require(void* module, const char* id) {
    NovaModule* mod = (NovaModule*)module;
    if (!mod || !id) return nullptr;

    // Use the module's path as the parent for resolution
    return nova_require_from(id, mod->filename);
}

int nova_module_isPreloading(void* module) {
    // Check if module is being preloaded
    // In Nova, we don't have preloading mechanism yet
    return 0;
}

// ============================================================================
// Module.register() for ESM loader hooks
// ============================================================================

void nova_module_register(const char* specifier, const char* parentURL) {
    // Register customization hooks for ESM loader
    // This is a placeholder - full implementation requires ESM support
    (void)specifier;
    (void)parentURL;
}

// ============================================================================
// Module.SourceMap class
// ============================================================================

struct NovaSourceMap {
    char* file;
    char* sourceRoot;
    char** sources;
    int sourcesCount;
    char** sourcesContent;
    char* mappings;
    char** names;
    int namesCount;
    int version;
};

void* nova_module_SourceMap_new(const char* payload) {
    NovaSourceMap* sm = new NovaSourceMap();
    sm->file = nullptr;
    sm->sourceRoot = nullptr;
    sm->sources = nullptr;
    sm->sourcesCount = 0;
    sm->sourcesContent = nullptr;
    sm->mappings = nullptr;
    sm->names = nullptr;
    sm->namesCount = 0;
    sm->version = 3;

    // Parse payload JSON (simplified - just store it)
    if (payload) {
        sm->mappings = allocString(payload);
    }

    return sm;
}

void nova_module_SourceMap_free(void* sourcemap) {
    NovaSourceMap* sm = (NovaSourceMap*)sourcemap;
    if (sm) {
        free(sm->file);
        free(sm->sourceRoot);
        free(sm->mappings);
        if (sm->sources) {
            for (int i = 0; i < sm->sourcesCount; i++) {
                free(sm->sources[i]);
            }
            free(sm->sources);
        }
        if (sm->sourcesContent) {
            for (int i = 0; i < sm->sourcesCount; i++) {
                free(sm->sourcesContent[i]);
            }
            free(sm->sourcesContent);
        }
        if (sm->names) {
            for (int i = 0; i < sm->namesCount; i++) {
                free(sm->names[i]);
            }
            free(sm->names);
        }
        delete sm;
    }
}

char* nova_module_SourceMap_payload(void* sourcemap) {
    NovaSourceMap* sm = (NovaSourceMap*)sourcemap;
    if (!sm || !sm->mappings) return nullptr;
    return allocString(sm->mappings);
}

void* nova_module_SourceMap_findEntry(void* sourcemap, int line, int column) {
    // Find source map entry for given line/column
    // Returns entry object with generatedLine, generatedColumn, originalSource, etc.
    (void)sourcemap;
    (void)line;
    (void)column;
    return nullptr;  // Simplified - would return entry object
}

void* nova_module_SourceMap_findOrigin(void* sourcemap, int line, int column) {
    // Find original source location
    (void)sourcemap;
    (void)line;
    (void)column;
    return nullptr;  // Simplified
}

// ============================================================================
// Additional Module static methods
// ============================================================================

char* nova_module_runMain() {
    // Run the main module
    // Returns the result
    return nullptr;
}

void* nova_module_globalPaths(int* count) {
    // Get global module paths
    if (!count) return nullptr;

    std::vector<std::string> paths;
#ifdef _WIN32
    char* appdata = getenv("APPDATA");
    if (appdata) {
        paths.push_back(std::string(appdata) + "\\npm\\node_modules");
    }
#else
    paths.push_back("/usr/local/lib/node_modules");
    paths.push_back("/usr/lib/node_modules");
#endif

    *count = (int)paths.size();
    if (*count == 0) return nullptr;

    char** result = (char**)malloc(sizeof(char*) * (*count));
    for (int i = 0; i < *count; i++) {
        result[i] = allocString(paths[i]);
    }
    return result;
}

int nova_module_isMainThread() {
    // Check if current thread is main thread
    return 1;  // Simplified - assume main thread
}

// ============================================================================
// Module private methods (internal use)
// ============================================================================

void* nova_module_load(void* module) {
    NovaModule* mod = (NovaModule*)module;
    if (!mod) return nullptr;

    // Load the module (implementation depends on file type)
    mod->loaded = true;
    return mod->exports;
}

void* nova_module_compile(void* module, const char* content, const char* filename) {
    NovaModule* mod = (NovaModule*)module;
    if (!mod) return nullptr;

    // Compile module content
    // In real implementation, this would parse and execute the code
    (void)content;
    (void)filename;

    mod->loaded = true;
    return mod->exports;
}

void nova_module_initPaths(void* module) {
    NovaModule* mod = (NovaModule*)module;
    if (!mod) return;

    // Initialize module lookup paths
    std::string dir = mod->path ? mod->path : ".";
    mod->paths = nodeModulePaths(dir);
}

// ============================================================================
// Cleanup
// ============================================================================

void nova_module_cleanup() {
    for (auto& pair : moduleCache) {
        NovaModule* mod = pair.second;
        if (mod != mainModule) {
            free(mod->id);
            free(mod->filename);
            free(mod->path);
            delete mod;
        }
    }
    moduleCache.clear();

    if (mainModule) {
        free(mainModule->id);
        free(mainModule->filename);
        free(mainModule->path);
        delete mainModule;
        mainModule = nullptr;
    }

    extensions.clear();
}

} // extern "C"
