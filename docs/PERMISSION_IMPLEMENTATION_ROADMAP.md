# Nova Permission System - Implementation Roadmap

**Date:** December 4, 2025
**Duration:** 8 weeks
**Status:** Ready to start

---

## Overview

This document provides a detailed week-by-week implementation plan for Nova's permission system. The plan is designed to be executed sequentially, with each week building on the previous week's work.

### Goals

- Implement secure-by-default permission system
- Integrate permission checks into all sensitive operations
- Provide CLI flags and runtime API
- Achieve 7.5/10 security score

### Timeline

**Total:** 8 weeks (December 4, 2025 - January 29, 2026)

---

## Week 1-2: Core Infrastructure

### Objectives

1. Implement core permission system classes
2. Add CLI flag parsing
3. Create basic permission checking logic
4. Set up testing infrastructure

### Tasks

#### Day 1-2: PermissionState Class

**File:** `src/runtime/Permissions.cpp`

```cpp
#include "nova/runtime/Permissions.h"
#include <iostream>

namespace nova {
namespace runtime {

// Singleton instance
PermissionState& PermissionState::getInstance() {
    static PermissionState instance;
    return instance;
}

// Constructor
PermissionState::PermissionState()
    : auditEnabled_(false),
      interactiveMode_(false) {
    // Check for audit mode
    const char* auditEnv = std::getenv("NOVA_AUDIT_PERMISSIONS");
    if (auditEnv && std::string(auditEnv) == "1") {
        auditEnabled_ = true;
        audit_ = std::make_unique<PermissionAudit>();
    }

    // Check if running in interactive mode (TTY)
    #ifdef _WIN32
    interactiveMode_ = _isatty(_fileno(stdin));
    #else
    interactiveMode_ = isatty(fileno(stdin));
    #endif

    if (interactiveMode_) {
        prompter_ = std::make_unique<PermissionPrompt>();
    }
}

PermissionState::~PermissionState() = default;

// Initialize from CLI
void PermissionState::initializeFromCLI(const std::vector<std::string>& args) {
    auto descriptors = CLIFlagParser::parse(args);
    for (const auto& desc : descriptors) {
        grant(desc);
    }
}

// Grant permission
void PermissionState::grant(const PermissionDescriptor& desc) {
    if (desc.target.has_value()) {
        // Specific permission
        specificPermissions_[desc.type].insert(*desc.target);
    } else {
        // Global permission
        globalPermissions_.insert(desc.type);
    }
}

// Query permission
PermissionStatus PermissionState::query(const PermissionDescriptor& desc) const {
    auto status = queryInternal(desc);

    // Log if audit enabled
    if (auditEnabled_ && audit_) {
        audit_->log(desc, status, "query");
    }

    return status;
}

// Internal query (no logging)
PermissionStatus PermissionState::queryInternal(const PermissionDescriptor& desc) const {
    // Check if explicitly denied
    if (deniedPermissions_.count(desc) > 0) {
        return PermissionStatus::Denied;
    }

    // Check global permission
    if (globalPermissions_.count(desc.type) > 0) {
        return PermissionStatus::Granted;
    }

    // Check specific permissions
    if (desc.target.has_value()) {
        if (matchesGrantedPermission(desc.type, *desc.target)) {
            return PermissionStatus::Granted;
        }
    }

    // If interactive, can prompt
    if (interactiveMode_ && prompter_ && prompter_->canPrompt()) {
        return PermissionStatus::Prompt;
    }

    return PermissionStatus::Denied;
}

// Check permission (throws if denied)
void PermissionState::check(const PermissionDescriptor& desc) {
    auto status = query(desc);

    if (status == PermissionStatus::Prompt) {
        // Try to prompt user
        status = request(desc);
    }

    if (status != PermissionStatus::Granted) {
        // Log denial
        if (auditEnabled_ && audit_) {
            audit_->log(desc, PermissionStatus::Denied, "");
        }

        // Throw exception
        throw PermissionDenied(desc.type, desc.target.value_or(""));
    }

    // Log grant
    if (auditEnabled_ && audit_) {
        audit_->log(desc, PermissionStatus::Granted, "check");
    }
}

// Request permission (may prompt)
PermissionStatus PermissionState::request(const PermissionDescriptor& desc) {
    auto status = queryInternal(desc);

    if (status == PermissionStatus::Granted) {
        return status;
    }

    if (status == PermissionStatus::Prompt && prompter_) {
        status = prompter_->prompt(desc);

        if (status == PermissionStatus::Granted) {
            // Remember for this session
            grant(desc);
        }
    }

    // Log
    if (auditEnabled_ && audit_) {
        audit_->log(desc, status, "request");
    }

    return status;
}

// Revoke permission
void PermissionState::revoke(const PermissionDescriptor& desc) {
    // Remove from granted
    if (desc.target.has_value()) {
        auto it = specificPermissions_.find(desc.type);
        if (it != specificPermissions_.end()) {
            it->second.erase(*desc.target);
        }
    } else {
        globalPermissions_.erase(desc.type);
    }

    // Add to denied
    deniedPermissions_.insert(desc);

    // Log
    if (auditEnabled_ && audit_) {
        audit_->log(desc, PermissionStatus::Denied, "revoke");
    }
}

// Get permission type name
std::string PermissionState::getPermissionTypeName(PermissionType type) {
    switch (type) {
        case PermissionType::Read: return "read";
        case PermissionType::Write: return "write";
        case PermissionType::Net: return "net";
        case PermissionType::Env: return "env";
        case PermissionType::Run: return "run";
        default: return "unknown";
    }
}

} // namespace runtime
} // namespace nova
```

#### Day 3-4: CLIFlagParser

**File:** `src/runtime/PermissionChecker.cpp` (partial)

```cpp
#include "nova/runtime/PermissionChecker.h"
#include <algorithm>
#include <sstream>

namespace nova {
namespace runtime {

std::vector<PermissionDescriptor> CLIFlagParser::parse(
    const std::vector<std::string>& args) {
    std::vector<PermissionDescriptor> descriptors;

    for (const auto& arg : args) {
        // Check for permission flags
        if (arg == "-A" || arg == "--allow-all") {
            // Grant all permissions
            descriptors.push_back({PermissionType::Read, std::nullopt});
            descriptors.push_back({PermissionType::Write, std::nullopt});
            descriptors.push_back({PermissionType::Net, std::nullopt});
            descriptors.push_back({PermissionType::Env, std::nullopt});
            descriptors.push_back({PermissionType::Run, std::nullopt});
        } else if (arg.starts_with("--allow-")) {
            auto flagDescriptors = parseFlag(arg);
            descriptors.insert(descriptors.end(),
                             flagDescriptors.begin(),
                             flagDescriptors.end());
        }
    }

    return descriptors;
}

std::vector<PermissionDescriptor> CLIFlagParser::parseFlag(
    const std::string& flag) {
    std::vector<PermissionDescriptor> descriptors;

    // Remove "--allow-" prefix
    std::string rest = flag.substr(8); // len("--allow-") = 8

    // Find = separator
    size_t eqPos = rest.find('=');

    std::string typeName;
    std::optional<std::string> values;

    if (eqPos != std::string::npos) {
        typeName = rest.substr(0, eqPos);
        values = rest.substr(eqPos + 1);
    } else {
        typeName = rest;
    }

    // Get permission type
    auto typeOpt = getPermissionType(typeName);
    if (!typeOpt.has_value()) {
        std::cerr << "Warning: Unknown permission flag: " << flag << std::endl;
        return descriptors;
    }

    auto type = *typeOpt;

    if (!values.has_value() || values->empty()) {
        // Global permission
        descriptors.push_back({type, std::nullopt});
    } else {
        // Parse comma-separated values
        auto targets = parseValues(*values);
        for (const auto& target : targets) {
            descriptors.push_back({type, target});
        }
    }

    return descriptors;
}

std::vector<std::string> CLIFlagParser::parseValues(const std::string& values) {
    std::vector<std::string> result;
    std::stringstream ss(values);
    std::string item;

    while (std::getline(ss, item, ',')) {
        // Trim whitespace
        item.erase(0, item.find_first_not_of(" \t"));
        item.erase(item.find_last_not_of(" \t") + 1);

        if (!item.empty()) {
            result.push_back(item);
        }
    }

    return result;
}

std::optional<PermissionType> CLIFlagParser::getPermissionType(
    const std::string& flagName) {
    if (flagName == "read") return PermissionType::Read;
    if (flagName == "write") return PermissionType::Write;
    if (flagName == "net") return PermissionType::Net;
    if (flagName == "env") return PermissionType::Env;
    if (flagName == "run") return PermissionType::Run;
    return std::nullopt;
}

} // namespace runtime
} // namespace nova
```

#### Day 5: Helper Functions

```cpp
// In Permissions.cpp

namespace nova {
namespace runtime {
namespace permissions {

void checkRead(const std::string& path) {
    PermissionState::getInstance().check({PermissionType::Read, path});
}

void checkWrite(const std::string& path) {
    PermissionState::getInstance().check({PermissionType::Write, path});
}

void checkNet(const std::string& host) {
    PermissionState::getInstance().check({PermissionType::Net, host});
}

void checkEnv(const std::string& variable) {
    PermissionState::getInstance().check({PermissionType::Env, variable});
}

void checkRun(const std::string& command) {
    PermissionState::getInstance().check({PermissionType::Run, command});
}

} // namespace permissions
} // namespace runtime
} // namespace nova
```

### Testing (Day 6-7)

Create test file: `tests/permission_test.cpp`

```cpp
#include "nova/runtime/Permissions.h"
#include <gtest/gtest.h>

using namespace nova::runtime;

TEST(PermissionStateTest, DefaultDeniesAll) {
    PermissionState state;

    EXPECT_THROW(
        state.check({PermissionType::Read, "/data"}),
        PermissionDenied
    );
}

TEST(PermissionStateTest, GlobalPermission) {
    PermissionState state;
    state.initializeFromCLI({"--allow-read"});

    EXPECT_NO_THROW(
        state.check({PermissionType::Read, "/data/file.txt"})
    );
}

TEST(PermissionStateTest, SpecificPermission) {
    PermissionState state;
    state.initializeFromCLI({"--allow-read=/data"});

    EXPECT_NO_THROW(
        state.check({PermissionType::Read, "/data/file.txt"})
    );

    EXPECT_THROW(
        state.check({PermissionType::Read, "/other/file.txt"}),
        PermissionDenied
    );
}

TEST(CLIFlagParserTest, ParseGlobalFlag) {
    auto descs = CLIFlagParser::parse({"--allow-read"});

    ASSERT_EQ(descs.size(), 1);
    EXPECT_EQ(descs[0].type, PermissionType::Read);
    EXPECT_FALSE(descs[0].target.has_value());
}

TEST(CLIFlagParserTest, ParseSpecificFlag) {
    auto descs = CLIFlagParser::parse({"--allow-read=/data"});

    ASSERT_EQ(descs.size(), 1);
    EXPECT_EQ(descs[0].type, PermissionType::Read);
    EXPECT_EQ(descs[0].target, "/data");
}

TEST(CLIFlagParserTest, ParseMultipleValues) {
    auto descs = CLIFlagParser::parse({"--allow-read=/data,/logs"});

    ASSERT_EQ(descs.size(), 2);
    EXPECT_EQ(descs[0].target, "/data");
    EXPECT_EQ(descs[1].target, "/logs");
}
```

### Deliverables

- ✅ Core permission classes implemented
- ✅ CLI flag parsing working
- ✅ Basic permission checking logic
- ✅ Unit tests passing

---

## Week 3: File System Protection

### Objectives

1. Implement path matching logic
2. Integrate checks into all FS operations
3. Test with real file operations

### Tasks

#### Day 1-2: PathMatcher Implementation

**File:** `src/runtime/PermissionChecker.cpp`

```cpp
bool PathMatcher::isAllowed(const std::string& grantedPath,
                            const std::string& requestPath) {
    // Resolve both to canonical absolute paths
    auto granted = resolvePath(grantedPath);
    auto request = resolvePath(requestPath);

    // Check if request is under granted directory
    if (request.starts_with(granted)) {
        // Make sure it's a directory boundary
        if (request.length() == granted.length() ||
            request[granted.length()] == '/' ||
            request[granted.length()] == '\\') {
            return true;
        }
    }

    return false;
}

std::string PathMatcher::resolvePath(const std::string& path) {
#ifdef _WIN32
    char buffer[MAX_PATH];
    if (GetFullPathNameA(path.c_str(), MAX_PATH, buffer, nullptr) == 0) {
        return path; // Fallback
    }
    return std::string(buffer);
#else
    char* resolved = realpath(path.c_str(), nullptr);
    if (resolved == nullptr) {
        return path; // Fallback
    }
    std::string result(resolved);
    free(resolved);
    return result;
#endif
}
```

#### Day 3-5: Integrate into BuiltinFS

**File:** `src/runtime/BuiltinFS.cpp` (modifications)

Add permission checks to every file system operation:

```cpp
#include "nova/runtime/Permissions.h"

// In fs.readFile()
llvm::Value* BuiltinFS::readFile(const std::string& path) {
    // CHECK PERMISSION FIRST
    nova::runtime::permissions::checkRead(path);

    // Original implementation continues...
    return performFileRead(path);
}

// In fs.writeFile()
llvm::Value* BuiltinFS::writeFile(const std::string& path,
                                   const std::string& data) {
    // CHECK PERMISSION
    nova::runtime::permissions::checkWrite(path);

    // Original implementation...
    return performFileWrite(path, data);
}

// Similarly for all other fs operations:
// - readFileSync, writeFileSync
// - readdir, readdirSync
// - mkdir, mkdirSync
// - rmdir, rmdirSync
// - unlink, unlinkSync
// - rename, renameSync
// - stat, statSync
// - open, close
// etc.
```

#### Day 6-7: Testing

Test all file system operations with permissions:

```typescript
// test_fs_permissions.ts

// Should fail without permission
try {
    const data = fs.readFileSync("/data/file.txt");
    console.log("ERROR: Should have thrown PermissionDenied");
} catch (e) {
    console.log("✅ Correctly denied read access");
}

// Test with permission
// Run: nova run --allow-read=/data test.ts
const data = fs.readFileSync("/data/file.txt");
console.log("✅ Read with permission succeeded");
```

### Deliverables

- ✅ Path matching working correctly
- ✅ All FS operations protected
- ✅ Tests passing
- ✅ No security bypasses found

---

## Week 4: Network Protection

### Objectives

1. Implement host matching logic
2. Integrate checks into HTTP/HTTPS/HTTP2
3. Test network operations

### Tasks

#### Day 1-2: HostMatcher Implementation

```cpp
bool HostMatcher::isAllowed(const std::string& grantedHost,
                            const std::string& requestHost) {
    auto [grantHost, grantPort] = parseHost(grantedHost);
    auto [reqHost, reqPort] = parseHost(requestHost);

    // Host must match exactly
    if (grantHost != reqHost) {
        return false;
    }

    // If no port specified in grant, allow all ports
    if (grantPort.empty()) {
        return true;
    }

    // Port must match
    return grantPort == reqPort;
}

std::pair<std::string, std::string> HostMatcher::parseHost(
    const std::string& hostPort) {
    // Handle IPv6: [::1]:8080
    if (hostPort[0] == '[') {
        size_t closeBracket = hostPort.find(']');
        if (closeBracket != std::string::npos) {
            std::string host = hostPort.substr(1, closeBracket - 1);
            if (closeBracket + 1 < hostPort.length() && hostPort[closeBracket + 1] == ':') {
                std::string port = hostPort.substr(closeBracket + 2);
                return {host, port};
            }
            return {host, ""};
        }
    }

    // Handle IPv4 / hostname: example.com:443
    size_t colon = hostPort.rfind(':');
    if (colon != std::string::npos) {
        std::string host = hostPort.substr(0, colon);
        std::string port = hostPort.substr(colon + 1);
        return {host, port};
    }

    return {hostPort, ""};
}

std::string HostMatcher::extractHost(const std::string& url) {
    // Simple URL parsing
    // Format: http://host:port/path
    // or https://host:port/path

    size_t schemeEnd = url.find("://");
    if (schemeEnd == std::string::npos) {
        return url;
    }

    size_t hostStart = schemeEnd + 3;
    size_t pathStart = url.find('/', hostStart);

    if (pathStart == std::string::npos) {
        return url.substr(hostStart);
    }

    return url.substr(hostStart, pathStart - hostStart);
}
```

#### Day 3-5: Integrate into HTTP Modules

```cpp
// In BuiltinHTTP.cpp
llvm::Value* BuiltinHTTP::fetch(const std::string& url) {
    // Extract and check host
    std::string host = HostMatcher::extractHost(url);
    nova::runtime::permissions::checkNet(host);

    // Original implementation...
    return performFetch(url);
}

// In http.createServer()
llvm::Value* BuiltinHTTP::createServer(/* args */) {
    // Check permission to listen
    // For localhost, check "localhost:port"
    std::string host = "localhost:" + std::to_string(port);
    nova::runtime::permissions::checkNet(host);

    // Original implementation...
}

// Similarly for HTTP2, HTTPS modules
```

### Deliverables

- ✅ Host matching working
- ✅ All network operations protected
- ✅ Tests passing

---

## Week 5: Environment & Subprocess

### Objectives

1. Protect environment variable access
2. Protect subprocess execution
3. Test all operations

### Tasks

#### Day 1-2: Environment Protection

```cpp
// In process.env getter
llvm::Value* getEnvVariable(const std::string& varName) {
    // Check permission
    nova::runtime::permissions::checkEnv(varName);

    // Get env var
    const char* value = std::getenv(varName.c_str());
    if (value == nullptr) {
        return nullptr;
    }
    return createString(value);
}
```

#### Day 3-4: Subprocess Protection

```cpp
// In child_process.exec(), spawn(), etc.
llvm::Value* execCommand(const std::string& command) {
    // Extract command name (first word)
    size_t spacePos = command.find(' ');
    std::string cmdName = command.substr(0, spacePos);

    // Check permission
    nova::runtime::permissions::checkRun(cmdName);

    // Execute
    return performExec(command);
}
```

### Deliverables

- ✅ Env and subprocess protected
- ✅ Tests passing

---

## Week 6: Runtime API & Config

### Objectives

1. Implement runtime permission APIs
2. Add interactive prompts
3. Support config file permissions

### Tasks

#### Day 1-3: Runtime API

Expose permission APIs to JavaScript:

```typescript
// Nova.permissions.query()
declare namespace Nova {
  namespace permissions {
    function query(desc: PermissionDescriptor): Promise<PermissionStatus>;
    function request(desc: PermissionDescriptor): Promise<PermissionStatus>;
    function revoke(desc: PermissionDescriptor): Promise<void>;
  }
}
```

#### Day 4-5: Config File Support

```cpp
// Load permissions from nova.json
void PermissionState::initializeFromConfig(const std::string& configPath) {
    // Parse JSON
    auto config = parseJSON(configPath);

    // Extract permissions section
    if (config.contains("permissions")) {
        auto perms = config["permissions"];

        // Parse each permission type
        if (perms.contains("read")) {
            parsePermissionArray(perms["read"], PermissionType::Read);
        }
        // ... similarly for write, net, env, run
    }
}
```

### Deliverables

- ✅ Runtime API working
- ✅ Config file support
- ✅ Tests passing

---

## Week 7-8: Audit & Polish

### Objectives

1. Implement audit logging
2. Security review
3. Documentation
4. Performance testing

### Tasks

#### Week 7: Audit & Security

1. Implement PermissionAudit class
2. Review all permission checks
3. Fuzzing tests
4. Security audit

#### Week 8: Documentation & Polish

1. Write user guide
2. Write API reference
3. Performance testing
4. Bug fixes

### Deliverables

- ✅ Audit logging working
- ✅ Security review complete
- ✅ Documentation complete
- ✅ Production ready

---

## Integration with main.cpp

Update CLI to initialize permissions:

```cpp
// In main.cpp
int main(int argc, char* argv[]) {
    // Initialize permission system
    std::vector<std::string> args(argv, argv + argc);
    nova::runtime::PermissionState::getInstance().initializeFromCLI(args);

    // Continue with normal execution...
}
```

---

## CMakeLists.txt Changes

Add new files to build:

```cmake
# Add permission system sources
set(NOVA_RUNTIME_SOURCES
    ${NOVA_RUNTIME_SOURCES}
    src/runtime/Permissions.cpp
    src/runtime/PermissionChecker.cpp
    src/runtime/PermissionAudit.cpp
    src/runtime/PermissionPrompt.cpp
)
```

---

## Success Criteria

### Functional

- [ ] All five permission types work (read, write, net, env, run)
- [ ] CLI flags parse correctly
- [ ] Path/host matching works correctly
- [ ] All FS operations protected
- [ ] All network operations protected
- [ ] Env and subprocess protected
- [ ] Runtime API working
- [ ] Config file support working
- [ ] Audit logging working

### Non-Functional

- [ ] Performance overhead < 0.1%
- [ ] Test coverage > 90%
- [ ] No security bypasses
- [ ] Clear documentation

### Security Score

**Target:** 3.4/10 → 7.5/10
- Permission System: 0/10 → 9/10 ✅
- Sandboxing: 0/10 → 7/10 ✅

---

## Next Steps After Completion

1. **Phase 2: npm Compatibility** (8-12 weeks)
   - Full node_modules resolution
   - Built-in APIs (fs, path, crypto)
   - Native addon support

2. **Phase 3: Developer Tools** (6-8 weeks)
   - Chrome DevTools debugger
   - Profiling tools
   - IDE integration

---

**Roadmap Status:** ✅ Ready to Execute
**Start Date:** December 4, 2025
**Target Completion:** January 29, 2026

---

*Implementation Roadmap v1.0 - Created December 4, 2025*
