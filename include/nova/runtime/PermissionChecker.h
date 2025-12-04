#ifndef NOVA_RUNTIME_PERMISSION_CHECKER_H
#define NOVA_RUNTIME_PERMISSION_CHECKER_H

#include "Permissions.h"
#include <string>
#include <vector>

namespace nova {
namespace runtime {

/**
 * Path matching logic for file system permissions
 */
class PathMatcher {
public:
    /**
     * Check if request path is allowed under granted path
     *
     * Examples:
     *   isAllowed("/data", "/data/file.txt") -> true
     *   isAllowed("/data", "/data/sub/file.txt") -> true
     *   isAllowed("/data", "/other/file.txt") -> false
     *   isAllowed("/data", "/data/../etc/passwd") -> false
     *
     * @param grantedPath Path that was granted permission
     * @param requestPath Path being requested
     * @return true if request is allowed
     */
    static bool isAllowed(const std::string& grantedPath,
                         const std::string& requestPath);

    /**
     * Resolve path to absolute canonical form
     * Resolves . and .. and symlinks
     *
     * @param path Path to resolve
     * @return Absolute canonical path
     */
    static std::string resolvePath(const std::string& path);

    /**
     * Normalize path (remove . and .. without resolving symlinks)
     *
     * @param path Path to normalize
     * @return Normalized path
     */
    static std::string normalizePath(const std::string& path);

private:
    // Check if path1 starts with path2 (directory prefix check)
    static bool isUnderDirectory(const std::string& path,
                                 const std::string& directory);
};

/**
 * Host matching logic for network permissions
 */
class HostMatcher {
public:
    /**
     * Check if request host:port is allowed under granted host:port
     *
     * Examples:
     *   isAllowed("example.com", "example.com:443") -> true (granted has no port)
     *   isAllowed("example.com:443", "example.com:443") -> true (exact match)
     *   isAllowed("example.com:443", "example.com:80") -> false (different port)
     *   isAllowed("example.com", "evil.com") -> false (different host)
     *
     * @param grantedHost Host (with optional port) that was granted
     * @param requestHost Host:port being requested
     * @return true if request is allowed
     */
    static bool isAllowed(const std::string& grantedHost,
                         const std::string& requestHost);

    /**
     * Parse host:port string into components
     *
     * Examples:
     *   parseHost("example.com") -> ("example.com", "")
     *   parseHost("example.com:443") -> ("example.com", "443")
     *   parseHost("192.168.1.1:8080") -> ("192.168.1.1", "8080")
     *   parseHost("[::1]:8080") -> ("::1", "8080")
     *
     * @param hostPort Host with optional port
     * @return Pair of (host, port) - port may be empty
     */
    static std::pair<std::string, std::string> parseHost(const std::string& hostPort);

    /**
     * Extract host from URL
     *
     * Examples:
     *   extractHost("http://example.com/path") -> "example.com"
     *   extractHost("https://example.com:443/path") -> "example.com:443"
     *
     * @param url Full URL
     * @return Host (with port if specified)
     */
    static std::string extractHost(const std::string& url);

private:
    // Check if IPv6 address
    static bool isIPv6(const std::string& host);
};

/**
 * CLI flag parser for permissions
 */
class CLIFlagParser {
public:
    /**
     * Parse permission flags from command-line arguments
     *
     * Recognized flags:
     *   --allow-read[=path]
     *   --allow-write[=path]
     *   --allow-net[=host]
     *   --allow-env[=var]
     *   --allow-run[=cmd]
     *   -A, --allow-all
     *
     * @param args Command-line arguments
     * @return Vector of permission descriptors to grant
     */
    static std::vector<PermissionDescriptor> parse(const std::vector<std::string>& args);

private:
    // Parse single flag
    static std::vector<PermissionDescriptor> parseFlag(const std::string& flag);

    // Parse comma-separated values
    static std::vector<std::string> parseValues(const std::string& values);

    // Get permission type from flag name
    static std::optional<PermissionType> getPermissionType(const std::string& flagName);
};

} // namespace runtime
} // namespace nova

#endif // NOVA_RUNTIME_PERMISSION_CHECKER_H
