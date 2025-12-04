#ifndef NOVA_RUNTIME_PERMISSION_AUDIT_H
#define NOVA_RUNTIME_PERMISSION_AUDIT_H

#include "Permissions.h"
#include <string>
#include <fstream>
#include <chrono>

namespace nova {
namespace runtime {

/**
 * Audit logger for permission checks
 *
 * Logs all permission checks to stderr in JSON format when
 * NOVA_AUDIT_PERMISSIONS environment variable is set.
 */
class PermissionAudit {
public:
    PermissionAudit();
    ~PermissionAudit();

    /**
     * Log a permission check
     *
     * @param desc Permission descriptor that was checked
     * @param status Result of permission check
     * @param source How permission was granted ("cli_flag", "config", "prompt", null)
     */
    void log(const PermissionDescriptor& desc,
             PermissionStatus status,
             const std::string& source = "");

    /**
     * Check if audit logging is enabled
     */
    bool isEnabled() const { return enabled_; }

private:
    // Format log entry as JSON
    std::string formatLogEntry(const PermissionDescriptor& desc,
                               PermissionStatus status,
                               const std::string& source,
                               const std::chrono::system_clock::time_point& timestamp);

    // Get current timestamp in ISO 8601 format
    std::string getCurrentTimestamp();

    bool enabled_;
    std::ofstream logFile_;
};

} // namespace runtime
} // namespace nova

#endif // NOVA_RUNTIME_PERMISSION_AUDIT_H
