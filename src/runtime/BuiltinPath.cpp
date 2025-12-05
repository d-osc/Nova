/**
 * nova:path - Path Module Implementation (Optimized)
 *
 * Provides path manipulation utilities for Nova programs.
 * Optimized for performance with direct C string operations.
 */

#include "nova/runtime/BuiltinModules.h"
#include <filesystem>
#include <cstring>
#include <cstdlib>
#include <sstream>

#ifdef _WIN32
#define PATH_SEP '\\'
#define PATH_DELIMITER ';'
#include <direct.h>
#define getcwd _getcwd
#else
#define PATH_SEP '/'
#define PATH_DELIMITER ':'
#include <unistd.h>
#endif

namespace nova {
namespace runtime {
namespace path {

// Fast inline helper to allocate and copy string
static inline char* allocString(const char* str, size_t len) {
    char* result = (char*)malloc(len + 1);
    if (result) {
        memcpy(result, str, len);
        result[len] = '\0';
    }
    return result;
}

// Overload for C++ strings (used by complex functions)
static inline char* allocString(const std::string& str) {
    return allocString(str.c_str(), str.length());
}

// Find last path separator - optimized backward scan
static inline const char* findLastSep(const char* path, size_t len) {
    for (const char* p = path + len - 1; p >= path; --p) {
        if (*p == '/' || *p == '\\') return p;
    }
    return nullptr;
}

// Find last dot for extension - only after last separator
static inline const char* findLastDot(const char* path, size_t len, const char* lastSep) {
    const char* start = lastSep ? lastSep + 1 : path;
    for (const char* p = path + len - 1; p >= start; --p) {
        if (*p == '.' && p > start) return p;
    }
    return nullptr;
}

extern "C" {

// Get directory name - OPTIMIZED
char* nova_path_dirname(const char* path) {
    if (!path) return allocString("", 0);

    size_t len = strlen(path);
    if (len == 0) return allocString(".", 1);

    const char* lastSep = findLastSep(path, len);
    if (!lastSep) return allocString(".", 1);

    // Handle root paths
    if (lastSep == path) return allocString("/", 1);

#ifdef _WIN32
    // Handle Windows drive root (C:\)
    if (lastSep == path + 2 && path[1] == ':') {
        return allocString(path, 3);
    }
#endif

    return allocString(path, lastSep - path);
}

// Get base name - OPTIMIZED
char* nova_path_basename(const char* path) {
    if (!path) return allocString("", 0);

    size_t len = strlen(path);
    if (len == 0) return allocString("", 0);

    const char* lastSep = findLastSep(path, len);
    if (!lastSep) return allocString(path, len);

    return allocString(lastSep + 1, len - (lastSep + 1 - path));
}

// Get extension (including dot) - OPTIMIZED
char* nova_path_extname(const char* path) {
    if (!path) return allocString("", 0);

    size_t len = strlen(path);
    if (len == 0) return allocString("", 0);

    const char* lastSep = findLastSep(path, len);
    const char* lastDot = findLastDot(path, len, lastSep);

    if (!lastDot) return allocString("", 0);

    return allocString(lastDot, len - (lastDot - path));
}

// Normalize path - OPTIMIZED with fast path
char* nova_path_normalize(const char* path) {
    if (!path) return allocString("", 0);

    size_t len = strlen(path);
    if (len == 0) return allocString(".", 1);

    // Fast path: if no "." components, just return as-is
    if (strstr(path, "/.") == nullptr && strstr(path, "\\.") == nullptr) {
        return allocString(path, len);
    }

    // Complex normalization - use std::filesystem
    std::filesystem::path p(path);
    return allocString(p.lexically_normal().string());
}

// Resolve to absolute path - OPTIMIZED with fast path
char* nova_path_resolve(const char* path) {
    if (!path) return allocString("", 0);

    size_t len = strlen(path);
    if (len == 0) {
        // Return current directory
        char cwd[4096];
        if (getcwd(cwd, sizeof(cwd))) {
            return allocString(cwd, strlen(cwd));
        }
        return allocString(".", 1);
    }

    // Fast path: if already absolute, just normalize
    bool isAbs = (path[0] == '/');
#ifdef _WIN32
    isAbs |= (len >= 2 && path[1] == ':');
#endif

    if (isAbs) {
        return nova_path_normalize(path);
    }

    // Relative path - resolve against cwd
    std::error_code ec;
    auto resolved = std::filesystem::absolute(path, ec);
    if (ec) {
        return allocString(path, len);
    }
    return allocString(resolved.lexically_normal().string());
}

// Check if path is absolute
int nova_path_isAbsolute(const char* path) {
    if (!path) return 0;

    size_t len = strlen(path);
    if (len == 0) return 0;

    // Unix absolute path
    if (path[0] == '/') return 1;

#ifdef _WIN32
    // Windows absolute path (C:\ or D:\ etc)
    if (len >= 2 && path[1] == ':') {
        char drive = path[0];
        if ((drive >= 'A' && drive <= 'Z') || (drive >= 'a' && drive <= 'z')) {
            return 1;
        }
    }

    // UNC path (\\server\share)
    if (len >= 2 && path[0] == '\\' && path[1] == '\\') {
        return 1;
    }
#endif

    return 0;
}

// Get relative path from 'from' to 'to'
char* nova_path_relative(const char* from, const char* to) {
    if (!from || !to) return allocString("", 0);

    std::filesystem::path fromPath(from);
    std::filesystem::path toPath(to);

    return allocString(toPath.lexically_relative(fromPath).string());
}

// Join path segments - OPTIMIZED for common cases
char* nova_path_join(const char** parts, int count) {
    if (!parts || count <= 0) return allocString("", 0);
    if (count == 1 && parts[0]) return allocString(parts[0], strlen(parts[0]));

    // Estimate total length to reduce reallocations
    [[maybe_unused]] size_t totalLen = 0;
    for (int i = 0; i < count; i++) {
        if (parts[i]) {
            totalLen += strlen(parts[i]) + 1; // +1 for separator
        }
    }

    // Fast path for 2 parts
    if (count == 2 && parts[0] && parts[1]) {
        size_t len0 = strlen(parts[0]);
        size_t len1 = strlen(parts[1]);
        bool needSep = (len0 > 0 && parts[0][len0-1] != '/' && parts[0][len0-1] != '\\');

        size_t resultLen = len0 + (needSep ? 1 : 0) + len1;
        char* result = (char*)malloc(resultLen + 1);
        if (result) {
            memcpy(result, parts[0], len0);
            if (needSep) result[len0] = PATH_SEP;
            memcpy(result + len0 + (needSep ? 1 : 0), parts[1], len1);
            result[resultLen] = '\0';
        }
        return result;
    }

    // General case - use std::filesystem
    std::filesystem::path result;
    for (int i = 0; i < count; i++) {
        if (parts[i]) {
            if (i == 0) {
                result = parts[i];
            } else {
                result /= parts[i];
            }
        }
    }

    return allocString(result.string());
}

// Path separator
char nova_path_sep() {
    return PATH_SEP;
}

// Path delimiter (for PATH env var)
char nova_path_delimiter() {
    return PATH_DELIMITER;
}

// Parse path into components - returns placeholder
void* nova_path_parse(const char* pathStr) {
    if (!pathStr) return nullptr;
    return (void*)pathStr;  // Placeholder - use individual functions
}

// Get root from path
char* nova_path_parse_root(const char* pathStr) {
    if (!pathStr) return allocString("", 0);
    std::filesystem::path p(pathStr);
    return allocString(p.root_path().string());
}

// Get dir from path (parent path)
char* nova_path_parse_dir(const char* pathStr) {
    if (!pathStr) return allocString("", 0);
    std::filesystem::path p(pathStr);
    return allocString(p.parent_path().string());
}

// Get base from path (filename with extension)
char* nova_path_parse_base(const char* pathStr) {
    if (!pathStr) return allocString("", 0);
    std::filesystem::path p(pathStr);
    return allocString(p.filename().string());
}

// Get name from path (filename without extension)
char* nova_path_parse_name(const char* pathStr) {
    if (!pathStr) return allocString("", 0);
    std::filesystem::path p(pathStr);
    return allocString(p.stem().string());
}

// Get ext from path
char* nova_path_parse_ext(const char* pathStr) {
    if (!pathStr) return allocString("", 0);
    std::filesystem::path p(pathStr);
    return allocString(p.extension().string());
}

// Format path object back to string
char* nova_path_format(const char* dir, const char* root, const char* base, const char* name, const char* ext) {
    std::string result;

    if (dir && strlen(dir) > 0) {
        result = dir;
        if (result.back() != PATH_SEP && result.back() != '/') {
            result += PATH_SEP;
        }
    } else if (root && strlen(root) > 0) {
        result = root;
    }

    if (base && strlen(base) > 0) {
        result += base;
    } else {
        if (name && strlen(name) > 0) {
            result += name;
        }
        if (ext && strlen(ext) > 0) {
            if (ext[0] != '.') result += ".";
            result += ext;
        }
    }

    return allocString(result);
}

// Convert to namespaced path (Windows only)
char* nova_path_toNamespacedPath(const char* pathStr) {
    if (!pathStr) return allocString("", 0);

#ifdef _WIN32
    std::string path(pathStr);
    // Convert to extended-length path if absolute
    if (path.length() >= 2 && path[1] == ':') {
        return allocString("\\\\?\\" + path);
    }
#endif
    return allocString(pathStr, strlen(pathStr));
}

// path.posix - use POSIX-style separators
char nova_path_posix_sep() {
    return '/';
}

char nova_path_posix_delimiter() {
    return ':';
}

// path.win32 - use Windows-style separators
char nova_path_win32_sep() {
    return '\\';
}

char nova_path_win32_delimiter() {
    return ';';
}

// Match patterns (glob-like) - simple implementation
int nova_path_matchesGlob(const char* pathStr, const char* pattern) {
    if (!pathStr || !pattern) return 0;

    std::string p(pathStr);
    std::string pat(pattern);
    if (pat == "*") return 1;
    if (pat.find('*') == std::string::npos) {
        return p == pat ? 1 : 0;
    }

    // Simple suffix match for *.ext patterns
    if (pat[0] == '*' && pat.length() > 1) {
        std::string suffix = pat.substr(1);
        if (p.length() >= suffix.length()) {
            return p.substr(p.length() - suffix.length()) == suffix ? 1 : 0;
        }
    }
    return 0;
}

} // extern "C"

} // namespace path
} // namespace runtime
} // namespace nova
