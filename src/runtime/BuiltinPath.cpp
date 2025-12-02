/**
 * nova:path - Path Module Implementation
 *
 * Provides path manipulation utilities for Nova programs.
 */

#include "nova/runtime/BuiltinModules.h"
#include <filesystem>
#include <cstring>
#include <cstdlib>
#include <sstream>

#ifdef _WIN32
#define PATH_SEP '\\'
#define PATH_DELIMITER ';'
#else
#define PATH_SEP '/'
#define PATH_DELIMITER ':'
#endif

namespace nova {
namespace runtime {
namespace path {

// Helper to allocate and copy string
static char* allocString(const std::string& str) {
    char* result = (char*)malloc(str.length() + 1);
    if (result) {
        strcpy(result, str.c_str());
    }
    return result;
}

extern "C" {

// Join path segments
char* nova_path_join(const char** parts, int count) {
    if (!parts || count <= 0) return allocString("");

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

// Get directory name
char* nova_path_dirname(const char* path) {
    if (!path) return allocString("");

    std::filesystem::path p(path);
    return allocString(p.parent_path().string());
}

// Get base name
char* nova_path_basename(const char* path) {
    if (!path) return allocString("");

    std::filesystem::path p(path);
    return allocString(p.filename().string());
}

// Get extension (including dot)
char* nova_path_extname(const char* path) {
    if (!path) return allocString("");

    std::filesystem::path p(path);
    return allocString(p.extension().string());
}

// Normalize path
char* nova_path_normalize(const char* path) {
    if (!path) return allocString("");

    std::filesystem::path p(path);
    return allocString(p.lexically_normal().string());
}

// Resolve to absolute path
char* nova_path_resolve(const char* path) {
    if (!path) return allocString("");

    std::error_code ec;
    auto resolved = std::filesystem::absolute(path, ec);
    if (ec) {
        return allocString(path);
    }
    return allocString(resolved.lexically_normal().string());
}

// Check if path is absolute
int nova_path_isAbsolute(const char* path) {
    if (!path) return 0;

    std::filesystem::path p(path);
    return p.is_absolute() ? 1 : 0;
}

// Get relative path from 'from' to 'to'
char* nova_path_relative(const char* from, const char* to) {
    if (!from || !to) return allocString("");

    std::filesystem::path fromPath(from);
    std::filesystem::path toPath(to);

    return allocString(toPath.lexically_relative(fromPath).string());
}

// Path separator
char nova_path_sep() {
    return PATH_SEP;
}

// Path delimiter (for PATH env var)
char nova_path_delimiter() {
    return PATH_DELIMITER;
}

// Parse path into components: { root, dir, base, ext, name }
// Returns JSON-like string for simplicity
void* nova_path_parse(const char* pathStr) {
    if (!pathStr) return nullptr;
    
    std::filesystem::path p(pathStr);
    // Returns pointer to static struct - in real impl would be proper object
    // For now we provide individual accessors
    return (void*)pathStr;  // Placeholder - use individual functions
}

// Get root from path
char* nova_path_parse_root(const char* pathStr) {
    if (!pathStr) return allocString("");
    std::filesystem::path p(pathStr);
    return allocString(p.root_path().string());
}

// Get dir from path (parent path)
char* nova_path_parse_dir(const char* pathStr) {
    if (!pathStr) return allocString("");
    std::filesystem::path p(pathStr);
    return allocString(p.parent_path().string());
}

// Get base from path (filename with extension)
char* nova_path_parse_base(const char* pathStr) {
    if (!pathStr) return allocString("");
    std::filesystem::path p(pathStr);
    return allocString(p.filename().string());
}

// Get name from path (filename without extension)
char* nova_path_parse_name(const char* pathStr) {
    if (!pathStr) return allocString("");
    std::filesystem::path p(pathStr);
    return allocString(p.stem().string());
}

// Get ext from path
char* nova_path_parse_ext(const char* pathStr) {
    if (!pathStr) return allocString("");
    std::filesystem::path p(pathStr);
    return allocString(p.extension().string());
}

// Format path object back to string
// Takes individual components
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

// Convert to namespaced path (Windows only, returns unchanged on other platforms)
char* nova_path_toNamespacedPath(const char* pathStr) {
    if (!pathStr) return allocString("");
    
#ifdef _WIN32
    std::string path(pathStr);
    // Convert to extended-length path if absolute
    if (path.length() >= 2 && path[1] == ':') {
        return allocString("\\\\?\\\\" + path);
    }
#endif
    return allocString(pathStr);
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

// Match patterns (glob-like)
int nova_path_matchesGlob(const char* pathStr, const char* pattern) {
    // Simple implementation - checks if pattern matches
    if (!pathStr || !pattern) return 0;
    // Basic wildcard matching
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
