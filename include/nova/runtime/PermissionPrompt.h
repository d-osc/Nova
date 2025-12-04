#ifndef NOVA_RUNTIME_PERMISSION_PROMPT_H
#define NOVA_RUNTIME_PERMISSION_PROMPT_H

#include "Permissions.h"
#include <string>

namespace nova {
namespace runtime {

/**
 * Interactive permission prompter
 *
 * Prompts user for permission when permission is not granted
 * and Nova is running in interactive mode (TTY).
 */
class PermissionPrompt {
public:
    PermissionPrompt();
    ~PermissionPrompt() = default;

    /**
     * Prompt user for permission
     *
     * Shows interactive prompt:
     *   ⚠️  script.ts requests <type> access to "<target>"
     *   ❯ Allow this time
     *     Allow always
     *     Deny
     *
     * @param desc Permission descriptor being requested
     * @return User's choice (Granted or Denied)
     */
    PermissionStatus prompt(const PermissionDescriptor& desc);

    /**
     * Check if interactive prompts are possible
     * Returns false if stdin/stdout is not a TTY
     */
    bool canPrompt() const { return canPrompt_; }

private:
    // Display prompt message
    void displayPrompt(const PermissionDescriptor& desc);

    // Get user input (arrow keys + enter)
    enum class Choice {
        AllowOnce,
        AllowAlways,
        Deny
    };
    Choice getUserChoice();

    // Format permission request message
    std::string formatRequestMessage(const PermissionDescriptor& desc);

    bool canPrompt_;  // true if stdin/stdout is TTY
};

} // namespace runtime
} // namespace nova

#endif // NOVA_RUNTIME_PERMISSION_PROMPT_H
