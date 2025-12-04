# Nova Permission System - Design Specification

**Date:** December 4, 2025
**Version:** 1.0
**Status:** Design Phase
**Target:** Phase 1 - Security Foundation

---

## Executive Summary

This document outlines the design for Nova's permission system, a critical security feature that will enable Nova to safely execute untrusted code. The design is based on Deno's proven architecture (10/10 security score) and Node.js 24's new native permission system, adapted for Nova's LLVM-based runtime.

### Goals

1. **Secure by Default** - No access to sensitive resources without explicit permission
2. **Fine-Grained Control** - Granular permissions for specific paths, hosts, and operations
3. **Easy to Use** - Simple CLI flags and runtime APIs
4. **Production Ready** - Battle-tested patterns from Deno and Node.js 24
5. **Performance** - Minimal overhead on runtime execution

### Security Score Impact

- **Current:** 3.4/10 (Critical gaps)
- **After Implementation:** 7.5/10 (Production ready)
- **Key Improvement:** Permission system 0/10 → 9/10

---

## Table of Contents

1. [Permission Types](#permission-types)
2. [Command-Line Interface](#command-line-interface)
3. [Runtime API](#runtime-api)
4. [Configuration File](#configuration-file)
5. [Internal Architecture](#internal-architecture)
6. [Implementation Plan](#implementation-plan)
7. [Testing Strategy](#testing-strategy)
8. [Migration Path](#migration-path)

---

## Permission Types

Nova will implement five core permission types, matching Deno's proven model:

### 1. File System Read (`--allow-read`)

**Purpose:** Control access to file system read operations

**Operations Protected:**
- `fs.readFile()`, `fs.readFileSync()`
- `fs.readdir()`, `fs.readdirSync()`
- `fs.stat()`, `fs.statSync()`
- `fs.open()` with read mode
- `fs.createReadStream()`

**Syntax:**
```bash
# Allow all read access
nova run --allow-read script.ts

# Allow specific directory
nova run --allow-read=/data script.ts

# Allow multiple paths (comma-separated)
nova run --allow-read=/data,/logs script.ts

# Allow specific file
nova run --allow-read=/config/app.json script.ts
```

**Behavior:**
- Without flag: All read operations throw `PermissionDenied` error
- With `--allow-read`: All reads allowed
- With `--allow-read=<path>`: Only reads under specified path(s) allowed
- Subdirectories are automatically included
- Symbolic links follow the target's permissions

---

### 2. File System Write (`--allow-write`)

**Purpose:** Control access to file system write operations

**Operations Protected:**
- `fs.writeFile()`, `fs.writeFileSync()`
- `fs.appendFile()`, `fs.appendFileSync()`
- `fs.mkdir()`, `fs.mkdirSync()`
- `fs.rmdir()`, `fs.rmdirSync()`
- `fs.unlink()`, `fs.unlinkSync()`
- `fs.rename()`, `fs.renameSync()`
- `fs.copyFile()`, `fs.copyFileSync()`
- `fs.createWriteStream()`

**Syntax:**
```bash
# Allow all write access
nova run --allow-write script.ts

# Allow specific directory
nova run --allow-write=/tmp script.ts

# Allow multiple paths
nova run --allow-write=/tmp,/data/output script.ts
```

**Behavior:**
- Without flag: All write operations throw `PermissionDenied` error
- Write implies read for the same path (can't write without checking existence)
- Subdirectories are automatically included

---

### 3. Network Access (`--allow-net`)

**Purpose:** Control access to network operations

**Operations Protected:**
- HTTP requests (`fetch()`, `http.request()`)
- HTTP servers (`http.createServer()`)
- TCP/UDP sockets
- WebSocket connections
- DNS resolution

**Syntax:**
```bash
# Allow all network access
nova run --allow-net script.ts

# Allow specific host
nova run --allow-net=api.example.com script.ts

# Allow specific host and port
nova run --allow-net=api.example.com:443 script.ts

# Allow multiple hosts
nova run --allow-net=api.example.com,db.example.com:5432 script.ts

# Allow IP addresses
nova run --allow-net=192.168.1.100:8080 script.ts

# Allow IPv6
nova run --allow-net=[2001:db8::1]:8080 script.ts

# Allow localhost (useful for development)
nova run --allow-net=localhost:3000 script.ts
```

**Behavior:**
- Without flag: All network operations throw `PermissionDenied` error
- Port can be omitted (allows all ports for that host)
- Wildcard domains not supported for security (must list specific hosts)
- Localhost is NOT implicitly allowed

---

### 4. Environment Variables (`--allow-env`)

**Purpose:** Control access to environment variables

**Operations Protected:**
- `process.env.VARIABLE`
- `process.env['VARIABLE']`
- Deno-style `Deno.env.get()`

**Syntax:**
```bash
# Allow all environment access
nova run --allow-env script.ts

# Allow specific variable
nova run --allow-env=DATABASE_URL script.ts

# Allow multiple variables
nova run --allow-env=DATABASE_URL,API_KEY,PORT script.ts
```

**Behavior:**
- Without flag: Reading any env var throws `PermissionDenied` error
- System env vars (PATH, HOME, etc.) may be allowed by default (TBD)
- Writing env vars always requires permission

---

### 5. Subprocess Execution (`--allow-run`)

**Purpose:** Control access to spawning subprocesses

**Operations Protected:**
- `child_process.exec()`
- `child_process.spawn()`
- `child_process.fork()`
- Any subprocess creation

**Syntax:**
```bash
# Allow all subprocess execution
nova run --allow-run script.ts

# Allow specific command
nova run --allow-run=git script.ts

# Allow multiple commands
nova run --allow-run=git,npm,node script.ts
```

**Behavior:**
- Without flag: All subprocess operations throw `PermissionDenied` error
- Only command name is checked, not arguments
- PATH resolution happens before permission check

---

### 6. All Permissions (`--allow-all` or `-A`)

**Purpose:** Convenience flag to grant all permissions

**Syntax:**
```bash
# Grant all permissions (development/trusted code only)
nova run --allow-all script.ts
nova run -A script.ts
```

**Behavior:**
- Equivalent to: `--allow-read --allow-write --allow-net --allow-env --allow-run`
- Should show a warning in console: "⚠️  Running with all permissions granted"
- NOT recommended for production

---

## Command-Line Interface

### Flag Design

All permission flags follow a consistent pattern:

```bash
--allow-<type>              # Grant all access for this type
--allow-<type>=<value>      # Grant specific access
--allow-<type>=<v1>,<v2>    # Grant multiple specific access
```

### Examples

**Web scraper (needs network + write):**
```bash
nova run --allow-net=example.com --allow-write=/data/scrape scraper.ts
```

**Build tool (needs read + write + subprocess):**
```bash
nova run --allow-read --allow-write --allow-run=git,npm build.ts
```

**API server (needs network + read + env):**
```bash
nova run --allow-net --allow-read=/config --allow-env=DATABASE_URL server.ts
```

**Development (all permissions):**
```bash
nova run -A dev.ts
```

---

## Runtime API

### Permission Query API

Check if permission is granted without prompting:

```typescript
// Query file system read permission
const status = await Nova.permissions.query({ name: "read", path: "/data" });
console.log(status.state); // "granted", "denied", or "prompt"

// Query network permission
const netStatus = await Nova.permissions.query({
  name: "net",
  host: "api.example.com"
});

// Query environment variable
const envStatus = await Nova.permissions.query({
  name: "env",
  variable: "DATABASE_URL"
});

// Query subprocess
const runStatus = await Nova.permissions.query({
  name: "run",
  command: "git"
});
```

### Permission Request API

Request permission at runtime (prompts user if not granted):

```typescript
// Request file read permission
const status = await Nova.permissions.request({ name: "read", path: "/data" });
if (status.state === "granted") {
  // Read files
}

// Request network permission
const netStatus = await Nova.permissions.request({
  name: "net",
  host: "api.example.com"
});
```

**Prompt Behavior:**
```
⚠️  script.ts requests network access to "api.example.com"
❯ Allow this time
  Allow always
  Deny
```

### Permission Revoke API

Revoke previously granted permission:

```typescript
// Revoke file read permission
await Nova.permissions.revoke({ name: "read", path: "/data" });

// Revoke network permission
await Nova.permissions.revoke({ name: "net", host: "api.example.com" });
```

### Permission Descriptor Types

```typescript
interface PermissionDescriptor {
  name: "read" | "write" | "net" | "env" | "run";
}

interface FilePermissionDescriptor extends PermissionDescriptor {
  name: "read" | "write";
  path?: string; // If omitted, checks global permission
}

interface NetPermissionDescriptor extends PermissionDescriptor {
  name: "net";
  host?: string; // e.g., "example.com:443"
}

interface EnvPermissionDescriptor extends PermissionDescriptor {
  name: "env";
  variable?: string;
}

interface RunPermissionDescriptor extends PermissionDescriptor {
  name: "run";
  command?: string;
}

interface PermissionStatus {
  state: "granted" | "denied" | "prompt";
  onchange?: (this: PermissionStatus, ev: Event) => any;
}
```

---

## Configuration File

Following Deno 2.5's approach, permissions can be specified in `nova.json`:

### Basic Configuration

```json
{
  "permissions": {
    "read": ["/data", "/config"],
    "write": ["/tmp", "/logs"],
    "net": ["api.example.com", "db.example.com:5432"],
    "env": ["DATABASE_URL", "API_KEY"],
    "run": ["git", "npm"]
  }
}
```

### Per-Command Permissions

```json
{
  "permissions": {
    "default": {
      "read": ["/config"]
    },
    "tasks": {
      "dev": {
        "read": true,
        "write": true,
        "net": true,
        "env": true,
        "run": ["npm", "node"]
      },
      "build": {
        "read": true,
        "write": ["/dist"],
        "run": ["git"]
      },
      "test": {
        "read": true,
        "write": ["/tmp"],
        "net": ["localhost:8080"]
      }
    }
  }
}
```

**Usage:**
```bash
# Uses dev permissions from config
nova run dev

# Uses build permissions from config
nova run build

# CLI flags override config
nova run build --allow-net
```

---

## Internal Architecture

### C++ Implementation Structure

```
include/nova/runtime/
├── Permissions.h         # Main permission system interface
├── PermissionChecker.h   # Permission checking logic
├── PermissionPrompt.h    # User prompt handling
└── PermissionAudit.h     # Audit logging

src/runtime/
├── Permissions.cpp       # Implementation
├── PermissionChecker.cpp
├── PermissionPrompt.cpp
└── PermissionAudit.cpp
```

### Core Classes

#### 1. PermissionState

```cpp
namespace nova {
namespace runtime {

enum class PermissionType {
    Read,
    Write,
    Net,
    Env,
    Run
};

enum class PermissionStatus {
    Granted,
    Denied,
    Prompt
};

struct PermissionDescriptor {
    PermissionType type;
    std::optional<std::string> target; // path, host, variable, or command
};

class PermissionState {
public:
    static PermissionState& getInstance();

    // Grant permissions from CLI flags
    void grantFromCLI(const std::vector<std::string>& args);

    // Check if permission is granted
    PermissionStatus check(const PermissionDescriptor& desc);

    // Request permission (may prompt user)
    PermissionStatus request(const PermissionDescriptor& desc);

    // Revoke permission
    void revoke(const PermissionDescriptor& desc);

    // Query permission without prompting
    PermissionStatus query(const PermissionDescriptor& desc);

private:
    // Global permissions (no target specified)
    std::unordered_set<PermissionType> globalPermissions_;

    // Specific permissions (with target)
    std::unordered_map<PermissionType, std::unordered_set<std::string>> specificPermissions_;

    // Audit log
    std::unique_ptr<PermissionAudit> audit_;
};

}} // namespace nova::runtime
```

#### 2. Permission Checking Integration

Every protected operation must check permissions:

```cpp
// In BuiltinFS.cpp - fs.readFile()
llvm::Value* BuiltinFS::readFile(const std::string& path) {
    // CHECK PERMISSION FIRST
    auto desc = PermissionDescriptor{
        .type = PermissionType::Read,
        .target = path
    };

    auto status = PermissionState::getInstance().check(desc);
    if (status != PermissionStatus::Granted) {
        throw PermissionDenied("Read access to '" + path + "' denied");
    }

    // Proceed with actual file read
    return performFileRead(path);
}

// In BuiltinHTTP.cpp - http.request()
llvm::Value* BuiltinHTTP::request(const std::string& url) {
    // Parse host from URL
    auto host = parseHost(url);

    // CHECK PERMISSION
    auto desc = PermissionDescriptor{
        .type = PermissionType::Net,
        .target = host
    };

    auto status = PermissionState::getInstance().check(desc);
    if (status != PermissionStatus::Granted) {
        throw PermissionDenied("Network access to '" + host + "' denied");
    }

    // Proceed with HTTP request
    return performHTTPRequest(url);
}
```

#### 3. Path Matching Logic

```cpp
class PathMatcher {
public:
    // Check if request path is allowed under granted path
    static bool isAllowed(const std::string& grantedPath,
                         const std::string& requestPath) {
        // Resolve to absolute paths
        auto granted = resolvePath(grantedPath);
        auto request = resolvePath(requestPath);

        // Check if request is under granted directory
        if (request.starts_with(granted)) {
            return true;
        }

        // Check if granted is a file and matches exactly
        if (granted == request) {
            return true;
        }

        return false;
    }
};
```

#### 4. Host Matching Logic

```cpp
class HostMatcher {
public:
    // Check if request host:port is allowed
    static bool isAllowed(const std::string& grantedHost,
                         const std::string& requestHost) {
        // Parse host:port
        auto [grantHost, grantPort] = parseHost(grantedHost);
        auto [reqHost, reqPort] = parseHost(requestHost);

        // Host must match exactly
        if (grantHost != reqHost) {
            return false;
        }

        // If granted port is empty, allow all ports
        if (grantPort.empty()) {
            return true;
        }

        // Port must match
        return grantPort == reqPort;
    }
};
```

---

## Implementation Plan

### Phase 1: Core Infrastructure (Week 1-2)

**Tasks:**
1. Create permission system classes
   - `Permissions.h/cpp`
   - `PermissionChecker.h/cpp`
   - `PermissionDescriptor` types

2. Implement CLI flag parsing
   - Parse `--allow-*` flags
   - Store in PermissionState singleton

3. Add error types
   - `PermissionDenied` exception
   - Error messages with helpful hints

**Deliverables:**
- Core permission classes
- CLI flag parsing working
- Unit tests for permission checking

---

### Phase 2: File System Protection (Week 3)

**Tasks:**
1. Integrate checks into BuiltinFS
   - `readFile`, `readFileSync`
   - `writeFile`, `writeFileSync`
   - `mkdir`, `rmdir`, `unlink`
   - All other fs operations

2. Implement path matching
   - Absolute path resolution
   - Subdirectory checking
   - Symlink handling

3. Write tests
   - Permission granted cases
   - Permission denied cases
   - Edge cases (symlinks, ../, etc.)

**Deliverables:**
- All fs operations protected
- Path matching working correctly
- Comprehensive test suite

---

### Phase 3: Network Protection (Week 4)

**Tasks:**
1. Integrate checks into BuiltinHTTP
   - `fetch()`
   - `http.createServer()`
   - `http.request()`

2. Integrate checks into BuiltinHTTP2, BuiltinHTTPS
   - All HTTP/HTTPS operations

3. Implement host matching
   - Parse host:port from URLs
   - Match against granted permissions
   - Handle IPv4, IPv6, hostnames

4. Write tests
   - Different host formats
   - Port matching
   - Denied requests

**Deliverables:**
- All network operations protected
- Host matching working
- Test suite

---

### Phase 4: Environment & Subprocess (Week 5)

**Tasks:**
1. Protect environment variable access
   - `process.env` getter
   - Runtime API

2. Protect subprocess execution
   - `child_process.exec()`
   - `child_process.spawn()`

3. Write tests

**Deliverables:**
- Env and subprocess protection
- Test suite

---

### Phase 5: Runtime API & Config (Week 6)

**Tasks:**
1. Implement runtime permission APIs
   - `Nova.permissions.query()`
   - `Nova.permissions.request()`
   - `Nova.permissions.revoke()`

2. Implement permission prompts
   - Interactive CLI prompts
   - Remember choice for session

3. Implement config file support
   - Parse `nova.json` permissions
   - Apply per-command permissions

4. Write tests

**Deliverables:**
- Full runtime API working
- Config file support
- Test suite

---

### Phase 6: Audit & Polish (Week 7-8)

**Tasks:**
1. Implement audit logging
   - `NOVA_AUDIT_PERMISSIONS` env var
   - Log all permission checks
   - JSON format for parsing

2. Security audit
   - Review all permission checks
   - Look for bypass opportunities
   - Fuzzing tests

3. Documentation
   - User guide
   - API reference
   - Migration guide

4. Performance testing
   - Measure overhead
   - Optimize hot paths

**Deliverables:**
- Audit logging working
- Security review complete
- Documentation complete
- Performance acceptable

---

## Testing Strategy

### Unit Tests

Test each component in isolation:

```typescript
// Permission checking logic
describe("PermissionChecker", () => {
  test("denies access without permission", () => {
    const state = new PermissionState();
    expect(() => state.check({ name: "read", path: "/data" }))
      .toThrow(PermissionDenied);
  });

  test("allows access with global permission", () => {
    const state = new PermissionState();
    state.grantFromCLI(["--allow-read"]);
    expect(state.check({ name: "read", path: "/data" }))
      .toBe(PermissionStatus.Granted);
  });

  test("allows subdirectories", () => {
    const state = new PermissionState();
    state.grantFromCLI(["--allow-read=/data"]);
    expect(state.check({ name: "read", path: "/data/sub/file.txt" }))
      .toBe(PermissionStatus.Granted);
  });
});

// Path matching
describe("PathMatcher", () => {
  test("matches subdirectories", () => {
    expect(PathMatcher.isAllowed("/data", "/data/sub/file.txt"))
      .toBe(true);
  });

  test("denies outside directory", () => {
    expect(PathMatcher.isAllowed("/data", "/other/file.txt"))
      .toBe(false);
  });

  test("handles .. correctly", () => {
    expect(PathMatcher.isAllowed("/data", "/data/../etc/passwd"))
      .toBe(false);
  });
});

// Host matching
describe("HostMatcher", () => {
  test("matches exact host:port", () => {
    expect(HostMatcher.isAllowed("example.com:443", "example.com:443"))
      .toBe(true);
  });

  test("allows all ports if not specified", () => {
    expect(HostMatcher.isAllowed("example.com", "example.com:443"))
      .toBe(true);
  });

  test("denies different hosts", () => {
    expect(HostMatcher.isAllowed("example.com", "evil.com"))
      .toBe(false);
  });
});
```

### Integration Tests

Test end-to-end scenarios:

```typescript
// File system access
test("fs.readFile requires permission", async () => {
  // Run without permission
  await expect(runNova("nova run --no-permissions test.ts"))
    .rejects.toThrow("PermissionDenied");

  // Run with permission
  await expect(runNova("nova run --allow-read test.ts"))
    .resolves.toBe(0);
});

// Network access
test("fetch requires permission", async () => {
  // Run without permission
  await expect(runNova("nova run fetch.ts"))
    .rejects.toThrow("PermissionDenied");

  // Run with permission
  await expect(runNova("nova run --allow-net=example.com fetch.ts"))
    .resolves.toBe(0);
});
```

### Security Tests

Test for bypass attempts:

```typescript
describe("Security", () => {
  test("cannot bypass with symbolic links", () => {
    // Create symlink from allowed dir to restricted dir
    fs.symlinkSync("/etc/passwd", "/tmp/link");

    // Should still be denied
    expect(() => runNova("nova run --allow-read=/tmp readlink.ts"))
      .toThrow(PermissionDenied);
  });

  test("cannot bypass with ../ traversal", () => {
    expect(() => runNova("nova run --allow-read=/tmp traversal.ts"))
      .toThrow(PermissionDenied);
  });

  test("cannot bypass with URL tricks", () => {
    // Try to bypass host check with URL encoding, etc.
    expect(() => runNova("nova run --allow-net=example.com bypass.ts"))
      .toThrow(PermissionDenied);
  });
});
```

### Fuzzing

Use fuzzing to find edge cases:

```cpp
// Fuzz test for path matching
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    std::string input(reinterpret_cast<const char*>(data), size);

    try {
        auto result = PathMatcher::isAllowed("/data", input);
        // Should never crash
    } catch (...) {
        return 0;
    }

    return 0;
}
```

---

## Migration Path

### Backward Compatibility

**Option 1: Strict (Recommended)**
- Default: All permissions denied
- Users must explicitly grant permissions
- May break existing scripts (but more secure)

**Option 2: Permissive (Migration Period)**
- Default: Show warnings but allow
- After 3 months: Switch to strict mode
- Gives users time to adapt

**Recommendation:** Use Option 1 (Strict) with clear error messages and documentation.

### Error Messages

Make error messages helpful:

```
Error: Permission denied: read access to "/data/file.txt"

This script requires file system read permission.
Run with: nova run --allow-read script.ts

For more information: https://nova-runtime.dev/docs/permissions
```

### Documentation

Create comprehensive guides:

1. **User Guide** - How to use permissions
2. **Migration Guide** - Updating existing scripts
3. **Security Best Practices** - When to grant permissions
4. **API Reference** - Runtime permission APIs

---

## Audit Logging

### Enable Audit Mode

```bash
# Log all permission checks
export NOVA_AUDIT_PERMISSIONS=1
nova run script.ts
```

### Audit Log Format

```json
{
  "timestamp": "2025-12-04T10:30:00Z",
  "type": "read",
  "target": "/data/file.txt",
  "granted": true,
  "source": "cli_flag"
}
{
  "timestamp": "2025-12-04T10:30:01Z",
  "type": "net",
  "target": "api.example.com:443",
  "granted": false,
  "source": null
}
```

### Use Cases

- **Security audits** - Review what resources scripts access
- **Debugging** - Understand permission denials
- **Compliance** - Log access to sensitive data

---

## Performance Considerations

### Overhead Analysis

**Permission check:**
- Hash map lookup: O(1)
- Path/host matching: O(n) where n = path length
- Expected overhead: < 1 microsecond per check

**Optimization strategies:**
1. Cache permission results for repeated checks
2. Fast path for global permissions (no matching needed)
3. Lazy initialization of PermissionState
4. Avoid string allocations in hot paths

### Benchmarks

Target performance:
- Permission check: < 1μs
- Path matching: < 2μs
- Host matching: < 2μs
- Total overhead: < 0.1% on real workloads

---

## Security Considerations

### Threat Model

**Threats:**
1. Malicious npm packages
2. Compromised dependencies
3. User-submitted scripts
4. Supply chain attacks

**Protection:**
- All permissions denied by default
- Fine-grained control
- Audit logging
- No privilege escalation

### Known Limitations

1. **Child processes inherit permissions** - Subprocess can do anything parent can
2. **No runtime permission revocation enforcement** - Code can cache granted permissions
3. **Prompt fatigue** - Too many prompts may cause users to always allow

### Future Enhancements

1. **Per-package permissions** - Different permissions for different npm packages
2. **Permission scoping** - Temporary permissions for specific operations
3. **Permission policies** - Organization-wide permission rules
4. **WASI integration** - WebAssembly System Interface for sandboxing

---

## Comparison with Other Runtimes

### vs Deno

**Similarities:**
- Same permission types (read, write, net, env, run)
- Same CLI flag syntax
- Runtime permission API

**Differences:**
- Nova: C++ implementation vs Deno's Rust
- Nova: Integrated into LLVM codegen
- Nova: Config file support from day 1 (Deno added in 2.5)

### vs Node.js 24

**Similarities:**
- Both adding native permission systems in 2024-2025
- Similar flag syntax

**Differences:**
- Nova: Secure by default (like Deno)
- Node.js 24: Opt-in permissions
- Nova: More granular per-resource control

### vs Bun

**Comparison:**
- Bun: No permission system (3/10 security score)
- Nova: Full permission system (7.5/10 target)

---

## Success Criteria

### Functional Requirements

- ✅ All five permission types implemented (read, write, net, env, run)
- ✅ CLI flags working
- ✅ Runtime API working
- ✅ Config file support
- ✅ Audit logging
- ✅ Secure by default

### Non-Functional Requirements

- ✅ Performance overhead < 0.1%
- ✅ No security bypasses found in testing
- ✅ Comprehensive test coverage (>90%)
- ✅ Clear documentation
- ✅ Migration guide complete

### Security Score Target

**Current:** 3.4/10
- Permission System: 0/10
- Sandboxing: 0/10
- Memory Safety: 4/10

**After Implementation:** 7.5/10
- Permission System: 9/10 ✅
- Sandboxing: 7/10 ✅ (permission-based)
- Memory Safety: 4/10 (unchanged, separate work)

---

## Timeline

**Total Duration:** 8 weeks

- **Weeks 1-2:** Core infrastructure
- **Week 3:** File system protection
- **Week 4:** Network protection
- **Week 5:** Env & subprocess protection
- **Week 6:** Runtime API & config
- **Weeks 7-8:** Audit, security review, docs

**Milestone Dates:**
- Week 2: Core permission system working
- Week 4: FS and network protection complete
- Week 6: Full feature set complete
- Week 8: Production ready

---

## Resources

### References

- [Deno Security Documentation](https://docs.deno.com/runtime/fundamentals/security/)
- [Deno Permission APIs](https://docs.deno.com/api/deno/permissions)
- [Node.js 24 Permission System](https://glinteco.com/en/post/embracing-nodejs-24s-built-in-tools-for-security-and-productivity-a-2025-guide/)
- [Deno 2.5 Config File Permissions](https://deno.com/blog/v2.5)
- [JavaScript Runtime Security Best Practices](https://healeycodes.com/sandboxing-javascript-code)
- [NodeShield Runtime Protection](https://arxiv.org/html/2508.13750v1)

### Team

- **Security Lead:** TBD
- **Implementation Lead:** TBD
- **Testing Lead:** TBD
- **Documentation:** TBD

---

## Appendix: Code Examples

### Example 1: Basic Permission Check

```cpp
// In any builtin function that needs permission
void checkPermission(PermissionType type, const std::string& target) {
    auto desc = PermissionDescriptor{ .type = type, .target = target };
    auto status = PermissionState::getInstance().check(desc);

    if (status != PermissionStatus::Granted) {
        std::string typeName = permissionTypeName(type);
        throw PermissionDenied(
            "Permission denied: " + typeName + " access to \"" + target + "\"\n\n" +
            "Run with: nova run --allow-" + typeName + " script.ts\n" +
            "For more info: https://nova-runtime.dev/docs/permissions"
        );
    }
}
```

### Example 2: User Scripts

```typescript
// test.ts - Needs read permission
const data = await fs.readFile("/data/file.txt");
console.log(data);

// Run with:
// nova run --allow-read=/data test.ts
```

```typescript
// server.ts - Needs network + env
const port = process.env.PORT || 3000;
const server = http.createServer((req, res) => {
  res.end("Hello World");
});
server.listen(port);

// Run with:
// nova run --allow-net --allow-env=PORT server.ts
```

---

**Document Status:** ✅ Design Complete
**Next Step:** Review with team, then begin implementation
**Target Start Date:** Week of December 4, 2025

---

*Design Document v1.0 - Created December 4, 2025*
