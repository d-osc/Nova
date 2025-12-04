#ifndef NOVA_RUNTIME_PERMISSIONS_H
#define NOVA_RUNTIME_PERMISSIONS_H

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <optional>
#include <memory>

namespace nova {
namespace runtime {

// Forward declarations
class PermissionAudit;
class PermissionPrompt;

/**
 * Permission types supported by Nova
 */
enum class PermissionType {
    Read,      // File system read access
    Write,     // File system write access
    Net,       // Network access
    Env,       // Environment variable access
    Run        // Subprocess execution
};

/**
 * Permission status after checking
 */
enum class PermissionStatus {
    Granted,   // Permission is granted
    Denied,    // Permission is denied
    Prompt     // Need to prompt user (interactive mode)
};

/**
 * Descriptor for a permission request
 */
struct PermissionDescriptor {
    PermissionType type;
    std::optional<std::string> target;  // Path, host, variable, or command

    // Equality for hash map
    bool operator==(const PermissionDescriptor& other) const {
        return type == other.type && target == other.target;
    }
};

} // namespace runtime
} // namespace nova

// Hash function for PermissionDescriptor
namespace std {
template<>
struct hash<nova::runtime::PermissionDescriptor> {
    size_t operator()(const nova::runtime::PermissionDescriptor& desc) const {
        size_t h1 = hash<int>()(static_cast<int>(desc.type));
        size_t h2 = desc.target ? hash<string>()(*desc.target) : 0;
        return h1 ^ (h2 << 1);
    }
};
} // namespace std

namespace nova {
namespace runtime {

/**
 * Central permission state manager (Singleton)
 *
 * This class manages all permission state for the Nova runtime.
 * It is initialized from CLI flags and can be queried at runtime.
 */
class PermissionState {
public:
    // Get singleton instance
    static PermissionState& getInstance();

    // Delete copy/move constructors (singleton)
    PermissionState(const PermissionState&) = delete;
    PermissionState& operator=(const PermissionState&) = delete;
    PermissionState(PermissionState&&) = delete;
    PermissionState& operator=(PermissionState&&) = delete;

    /**
     * Initialize permissions from command-line arguments
     * Called once at startup with CLI flags
     *
     * Examples:
     *   --allow-read              -> Grant all read access
     *   --allow-read=/data        -> Grant read to /data and subdirs
     *   --allow-net=example.com   -> Grant net to example.com
     *   -A or --allow-all         -> Grant all permissions
     */
    void initializeFromCLI(const std::vector<std::string>& args);

    /**
     * Initialize permissions from config file (nova.json)
     */
    void initializeFromConfig(const std::string& configPath);

    /**
     * Check if a permission is granted
     * Throws PermissionDenied exception if denied
     *
     * @param desc Permission descriptor
     * @throws PermissionDenied if permission is not granted
     */
    void check(const PermissionDescriptor& desc);

    /**
     * Query permission status without throwing
     *
     * @param desc Permission descriptor
     * @return PermissionStatus (Granted, Denied, or Prompt)
     */
    PermissionStatus query(const PermissionDescriptor& desc) const;

    /**
     * Request permission (may prompt user in interactive mode)
     *
     * @param desc Permission descriptor
     * @return PermissionStatus after request
     */
    PermissionStatus request(const PermissionDescriptor& desc);

    /**
     * Revoke a previously granted permission
     *
     * @param desc Permission descriptor to revoke
     */
    void revoke(const PermissionDescriptor& desc);

    /**
     * Check if audit logging is enabled
     */
    bool isAuditEnabled() const { return auditEnabled_; }

    /**
     * Enable/disable audit logging
     */
    void setAuditEnabled(bool enabled);

    /**
     * Get permission type name as string
     */
    static std::string getPermissionTypeName(PermissionType type);

private:
    PermissionState();
    ~PermissionState();

    // Internal query without audit logging
    PermissionStatus queryInternal(const PermissionDescriptor& desc) const;

    // Parse CLI flag and grant permissions
    void parseCLIFlag(const std::string& flag);

    // Grant permission
    void grant(const PermissionDescriptor& desc);

    // Check if target matches granted permission
    bool matchesGrantedPermission(PermissionType type,
                                   const std::string& target) const;

    // Global permissions (no specific target)
    // e.g., --allow-read grants read access to everything
    std::unordered_set<PermissionType> globalPermissions_;

    // Specific permissions with targets
    // e.g., --allow-read=/data grants read to /data only
    std::unordered_map<PermissionType, std::unordered_set<std::string>> specificPermissions_;

    // Denied permissions (from revoke())
    std::unordered_set<PermissionDescriptor> deniedPermissions_;

    // Audit logger
    std::unique_ptr<PermissionAudit> audit_;
    bool auditEnabled_;

    // Permission prompter (for interactive mode)
    std::unique_ptr<PermissionPrompt> prompter_;
    bool interactiveMode_;
};

/**
 * Exception thrown when permission is denied
 */
class PermissionDenied : public std::runtime_error {
public:
    PermissionDenied(const std::string& message)
        : std::runtime_error(message) {}

    PermissionDenied(PermissionType type, const std::string& target)
        : std::runtime_error(formatMessage(type, target)) {}

private:
    static std::string formatMessage(PermissionType type,
                                     const std::string& target);
};

/**
 * Helper functions for permission checking in builtin modules
 */
namespace permissions {

/**
 * Check file system read permission
 * @throws PermissionDenied if not granted
 */
void checkRead(const std::string& path);

/**
 * Check file system write permission
 * @throws PermissionDenied if not granted
 */
void checkWrite(const std::string& path);

/**
 * Check network access permission
 * @throws PermissionDenied if not granted
 */
void checkNet(const std::string& host);

/**
 * Check environment variable access permission
 * @throws PermissionDenied if not granted
 */
void checkEnv(const std::string& variable);

/**
 * Check subprocess execution permission
 * @throws PermissionDenied if not granted
 */
void checkRun(const std::string& command);

} // namespace permissions

} // namespace runtime
} // namespace nova

#endif // NOVA_RUNTIME_PERMISSIONS_H
