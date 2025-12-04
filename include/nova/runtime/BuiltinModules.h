#pragma once

/**
 * Nova Built-in Modules
 *
 * Provides built-in modules with nova: prefix:
 * - nova:fs    - File system operations (100% Node.js compatible)
 * - nova:test  - Testing utilities (describe, test, expect)
 * - nova:path  - Path manipulation
 * - nova:os    - OS utilities
 */

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace nova {
namespace runtime {

// Forward declarations for runtime value types
struct NovaValue;

// ============================================================================
// nova:fs - File System Module (Node.js Compatible)
// https://nodejs.org/api/fs.html
// ============================================================================

namespace fs {

extern "C" {
    // ---------- File Access & Metadata ----------

    // fs.accessSync(path[, mode]) - Tests user permissions
    int nova_fs_accessSync(const char* path, int mode);

    // fs.existsSync(path) - Determines if a path exists
    int nova_fs_existsSync(const char* path);

    // fs.statSync(path) - Returns file statistics (NovaStats*)
    void* nova_fs_statSync(const char* path);

    // fs.lstatSync(path) - Returns symbolic link statistics
    void* nova_fs_lstatSync(const char* path);

    // fs.realpathSync(path) - Resolves canonical path
    char* nova_fs_realpathSync(const char* path);

    // ---------- File Operations ----------

    // fs.readFileSync(path[, options]) - Reads entire file
    char* nova_fs_readFileSync(const char* path);
    char* nova_fs_readFileSyncEncoding(const char* path, const char* encoding);

    // fs.writeFileSync(file, data[, options]) - Writes data to file
    int nova_fs_writeFileSync(const char* path, const char* data);

    // fs.appendFileSync(path, data[, options]) - Appends data to file
    int nova_fs_appendFileSync(const char* path, const char* data);

    // fs.truncateSync(path[, len]) - Truncates a file
    int nova_fs_truncateSync(const char* path, int64_t len);

    // fs.copyFileSync(src, dest[, mode]) - Copies a file
    int nova_fs_copyFileSync(const char* src, const char* dest);

    // fs.cpSync(src, dest[, options]) - Recursively copies files/directories
    int nova_fs_cpSync(const char* src, const char* dest);

    // ---------- File Management ----------

    // fs.unlinkSync(path) - Deletes a file
    int nova_fs_unlinkSync(const char* path);

    // fs.renameSync(oldPath, newPath) - Renames a file
    int nova_fs_renameSync(const char* oldPath, const char* newPath);

    // fs.rmSync(path[, options]) - Recursively removes files/directories
    int nova_fs_rmSync(const char* path);
    int nova_fs_rmSyncOptions(const char* path, int recursive, int force);

    // ---------- Symbolic Links ----------

    // fs.linkSync(existingPath, newPath) - Creates hard link
    int nova_fs_linkSync(const char* existingPath, const char* newPath);

    // fs.symlinkSync(target, path[, type]) - Creates symbolic link
    int nova_fs_symlinkSync(const char* target, const char* path);

    // fs.readlinkSync(path) - Reads symbolic link
    char* nova_fs_readlinkSync(const char* path);

    // ---------- Directory Operations ----------

    // fs.readdirSync(path[, options]) - Lists directory contents
    char* nova_fs_readdirSync(const char* path);
    char** nova_fs_readdirSyncArray(const char* path, int* count);

    // fs.mkdirSync(path[, options]) - Creates a directory
    int nova_fs_mkdirSync(const char* path);
    int nova_fs_mkdirSyncOptions(const char* path, int recursive, int mode);

    // fs.rmdirSync(path) - Removes a directory
    int nova_fs_rmdirSync(const char* path);

    // fs.mkdtempSync(prefix[, options]) - Creates temporary directory
    char* nova_fs_mkdtempSync(const char* prefix);

    // ---------- Permissions & Ownership ----------

    // fs.chmodSync(path, mode) - Changes file permissions
    int nova_fs_chmodSync(const char* path, int mode);

    // fs.chownSync(path, uid, gid) - Changes file ownership
    int nova_fs_chownSync(const char* path, int uid, int gid);

    // fs.lchownSync(path, uid, gid) - Changes symbolic link ownership
    int nova_fs_lchownSync(const char* path, int uid, int gid);

    // ---------- Time Operations ----------

    // fs.utimesSync(path, atime, mtime) - Changes file timestamps
    int nova_fs_utimesSync(const char* path, double atime, double mtime);

    // ---------- File Descriptor Operations ----------

    // fs.openSync(path, flags[, mode]) - Opens a file
    int nova_fs_openSync(const char* path, const char* flags);

    // fs.closeSync(fd) - Closes a file descriptor
    int nova_fs_closeSync(int fd);

    // fs.readSync(fd, buffer, length, position) - Reads from file descriptor
    int64_t nova_fs_readSync(int fd, char* buffer, int64_t length, int64_t position);

    // fs.writeSync(fd, buffer, length, position) - Writes to file descriptor
    int64_t nova_fs_writeSync(int fd, const char* buffer, int64_t length, int64_t position);

    // fs.fsyncSync(fd) - Flushes data to storage
    int nova_fs_fsyncSync(int fd);

    // fs.ftruncateSync(fd, len) - Truncates file via descriptor
    int nova_fs_ftruncateSync(int fd, int64_t len);

    // fs.fstatSync(fd) - Returns file statistics from descriptor
    void* nova_fs_fstatSync(int fd);

    // fs.fchmodSync(fd, mode) - Changes permissions via descriptor
    int nova_fs_fchmodSync(int fd, int mode);

    // fs.fchownSync(fd, uid, gid) - Changes ownership via descriptor
    int nova_fs_fchownSync(int fd, int uid, int gid);

    // fs.fdatasyncSync(fd) - Flushes data only (not metadata)
    int nova_fs_fdatasyncSync(int fd);

    // fs.futimesSync(fd, atime, mtime) - Changes timestamps via descriptor
    int nova_fs_futimesSync(int fd, double atime, double mtime);

    // ---------- Stats Helper Functions ----------

    int64_t nova_fs_stats_size(void* stats);
    int64_t nova_fs_stats_mode(void* stats);
    double nova_fs_stats_mtimeMs(void* stats);
    double nova_fs_stats_atimeMs(void* stats);
    double nova_fs_stats_ctimeMs(void* stats);
    double nova_fs_stats_birthtimeMs(void* stats);
    int nova_fs_stats_isFile(void* stats);
    int nova_fs_stats_isDirectory(void* stats);
    int nova_fs_stats_isSymbolicLink(void* stats);
    int nova_fs_stats_isBlockDevice(void* stats);
    int nova_fs_stats_isCharacterDevice(void* stats);
    int nova_fs_stats_isFIFO(void* stats);
    int nova_fs_stats_isSocket(void* stats);
    void nova_fs_stats_free(void* stats);

    // ---------- Constants ----------

    int nova_fs_constants_F_OK();  // File exists
    int nova_fs_constants_R_OK();  // File is readable
    int nova_fs_constants_W_OK();  // File is writable
    int nova_fs_constants_X_OK();  // File is executable
    int nova_fs_constants_COPYFILE_EXCL();
    int nova_fs_constants_COPYFILE_FICLONE();
    int nova_fs_constants_COPYFILE_FICLONE_FORCE();

    // Open flags constants
    int nova_fs_constants_O_RDONLY();
    int nova_fs_constants_O_WRONLY();
    int nova_fs_constants_O_RDWR();
    int nova_fs_constants_O_CREAT();
    int nova_fs_constants_O_EXCL();
    int nova_fs_constants_O_TRUNC();
    int nova_fs_constants_O_APPEND();

    // ---------- Directory Iterator (opendirSync) ----------

    // fs.opendirSync(path) - Opens a directory for iteration
    void* nova_fs_opendirSync(const char* path);

    // dir.readSync() - Reads next entry name
    char* nova_fs_dir_readSync(void* dir);

    // dir.readSync() - Reads next entry as Dirent
    void* nova_fs_dir_readSyncDirent(void* dir);

    // dir.closeSync() - Closes directory
    int nova_fs_dir_closeSync(void* dir);

    // Dirent helper functions
    char* nova_fs_dirent_name(void* dirent);
    int nova_fs_dirent_isFile(void* dirent);
    int nova_fs_dirent_isDirectory(void* dirent);
    int nova_fs_dirent_isSymbolicLink(void* dirent);
    void nova_fs_dirent_free(void* dirent);

    // ---------- Filesystem Statistics (statfsSync) ----------

    // fs.statfsSync(path) - Returns filesystem statistics
    void* nova_fs_statfsSync(const char* path);

    // StatFs helper functions
    int64_t nova_fs_statfs_bsize(void* statfs);
    int64_t nova_fs_statfs_blocks(void* statfs);
    int64_t nova_fs_statfs_bfree(void* statfs);
    int64_t nova_fs_statfs_bavail(void* statfs);
    void nova_fs_statfs_free(void* statfs);

    // ---------- Glob Pattern Matching (globSync) ----------

    // fs.globSync(pattern) - Glob pattern matching (Node.js 22+)
    char* nova_fs_globSync(const char* pattern);
    char* nova_fs_globSyncOptions(const char* pattern, const char* cwd);

    // ---------- Watch (stubs) ----------

    void* nova_fs_watchFile(const char* path, void (*callback)(void*));
    int nova_fs_unwatchFile(const char* path);

    // ---------- Additional Node.js fs Sync Functions ----------

    // fs.lchmodSync(path, mode) - Change symlink permissions (macOS)
    int nova_fs_lchmodSync(const char* path, int mode);

    // fs.lutimesSync(path, atime, mtime) - Change symlink timestamps
    int nova_fs_lutimesSync(const char* path, double atime, double mtime);

    // fs.readvSync(fd, buffers, lengths, count, position) - Scatter read
    int64_t nova_fs_readvSync(int fd, char** buffers, int64_t* lengths, int count, int64_t position);

    // fs.writevSync(fd, buffers, lengths, count, position) - Gather write
    int64_t nova_fs_writevSync(int fd, const char** buffers, int64_t* lengths, int count, int64_t position);

    // fs.realpathSync.native(path) - Native realpath
    char* nova_fs_realpathSyncNative(const char* path);

    // fs.mkdtempDisposableSync(prefix) - Temp directory (Node.js 22+)
    char* nova_fs_mkdtempDisposableSync(const char* prefix);
}

} // namespace fs

// ============================================================================
// nova:test - Testing Module
// ============================================================================

namespace test {

// Test result structure
struct TestResult {
    std::string name;
    bool passed;
    std::string error;
    double durationMs;
};

// Test suite structure
struct TestSuite {
    std::string name;
    std::vector<TestResult> tests;
    int passed;
    int failed;
    int skipped;
};

// Global test state
extern std::vector<TestSuite> g_testSuites;
extern TestSuite* g_currentSuite;
extern int g_totalPassed;
extern int g_totalFailed;

extern "C" {
    // describe(name, fn) - Create a test suite
    void nova_test_describe(const char* name, void (*fn)());

    // test(name, fn) / it(name, fn) - Create a test case
    void nova_test_test(const char* name, void (*fn)());
    void nova_test_it(const char* name, void (*fn)());

    // beforeEach/afterEach hooks
    void nova_test_beforeEach(void (*fn)());
    void nova_test_afterEach(void (*fn)());

    // beforeAll/afterAll hooks
    void nova_test_beforeAll(void (*fn)());
    void nova_test_afterAll(void (*fn)());

    // Assertions - expect(value)
    // Returns assertion context for chaining
    void* nova_test_expect(double value);
    void* nova_test_expect_str(const char* value);
    void* nova_test_expect_bool(int value);
    void* nova_test_expect_ptr(void* value);

    // Matchers
    void nova_test_toBe(void* ctx, double expected);
    void nova_test_toBe_str(void* ctx, const char* expected);
    void nova_test_toEqual(void* ctx, double expected);
    void nova_test_toBeTruthy(void* ctx);
    void nova_test_toBeFalsy(void* ctx);
    void nova_test_toBeNull(void* ctx);
    void nova_test_toBeUndefined(void* ctx);
    void nova_test_toBeGreaterThan(void* ctx, double expected);
    void nova_test_toBeLessThan(void* ctx, double expected);
    void nova_test_toBeGreaterThanOrEqual(void* ctx, double expected);
    void nova_test_toBeLessThanOrEqual(void* ctx, double expected);
    void nova_test_toContain(void* ctx, const char* expected);
    void nova_test_toHaveLength(void* ctx, int expected);
    void nova_test_toThrow(void* ctx);

    // Negation - expect(x).not.toBe(y)
    void* nova_test_not(void* ctx);

    // Run all tests and return exit code
    int nova_test_runAll();

    // Skip test
    void nova_test_skip(const char* name, void (*fn)());

    // Only run this test
    void nova_test_only(const char* name, void (*fn)());
}

} // namespace test

// ============================================================================
// nova:path - Path Module
// ============================================================================

namespace path {

extern "C" {
    // Join path segments
    char* nova_path_join(const char** parts, int count);

    // Get directory name
    char* nova_path_dirname(const char* path);

    // Get base name
    char* nova_path_basename(const char* path);

    // Get extension
    char* nova_path_extname(const char* path);

    // Normalize path
    char* nova_path_normalize(const char* path);

    // Resolve to absolute path
    char* nova_path_resolve(const char* path);

    // Check if path is absolute
    int nova_path_isAbsolute(const char* path);

    // Get relative path
    char* nova_path_relative(const char* from, const char* to);

    // Path separator
    char nova_path_sep();
    char nova_path_delimiter();
}

} // namespace path

// ============================================================================
// nova:os - OS Module
// ============================================================================

namespace os {

extern "C" {
    // Get platform name
    char* nova_os_platform();

    // Get architecture
    char* nova_os_arch();

    // Get home directory
    char* nova_os_homedir();

    // Get temp directory
    char* nova_os_tmpdir();

    // Get hostname
    char* nova_os_hostname();

    // Get current working directory
    char* nova_os_cwd();

    // Change directory
    int nova_os_chdir(const char* path);

    // Get environment variable
    char* nova_os_getenv(const char* name);

    // Set environment variable
    int nova_os_setenv(const char* name, const char* value);

    // Get CPU count
    int nova_os_cpus();

    // Get total memory
    long long nova_os_totalmem();

    // Get free memory
    long long nova_os_freemem();

    // Get uptime
    double nova_os_uptime();

    // Exit process
    void nova_os_exit(int code);
}

} // namespace os

// ============================================================================
// nova:http - HTTP Module
// ============================================================================

namespace http {

extern "C" {
    // Server functions
    void* nova_http_createServer(void* requestListener);
    int nova_http_Server_listen(void* serverPtr, int port, const char* hostname, void* callback);
    void nova_http_Server_close(void* serverPtr, void* callback);
    int nova_http_Server_listening(void* serverPtr);
    int nova_http_Server_acceptOne(void* serverPtr, int timeoutMs);
    int nova_http_Server_run(void* serverPtr, int maxRequests);
    void nova_http_Server_free(void* serverPtr);

    // Response functions (passed to request handler, don't create directly)
    void nova_http_ServerResponse_setStatusCode(void* resPtr, int code);
    int nova_http_ServerResponse_statusCode(void* resPtr);
    void nova_http_ServerResponse_setHeader(void* resPtr, const char* name, const char* value);
    char* nova_http_ServerResponse_getHeader(void* resPtr, const char* name);
    int nova_http_ServerResponse_write(void* resPtr, const char* data, int length);
    void nova_http_ServerResponse_end(void* resPtr, const char* data, int length);

    // Request functions (passed to request handler)
    char* nova_http_IncomingMessage_method(void* msgPtr);
    char* nova_http_IncomingMessage_url(void* msgPtr);
    char* nova_http_IncomingMessage_getHeader(void* msgPtr, const char* name);

    // Utility functions
    char* nova_http_STATUS_CODES(int code);
}

} // namespace http

// ============================================================================
// nova:http2 - HTTP/2 Module
// ============================================================================

namespace http2 {

extern "C" {
    // ============================================================================
    // Constants Export
    // ============================================================================

    int nova_http2_constants_NO_ERROR();
    int nova_http2_constants_PROTOCOL_ERROR();
    int nova_http2_constants_INTERNAL_ERROR();
    int nova_http2_constants_FLOW_CONTROL_ERROR();
    int nova_http2_constants_SETTINGS_TIMEOUT();
    int nova_http2_constants_STREAM_CLOSED();
    int nova_http2_constants_FRAME_SIZE_ERROR();
    int nova_http2_constants_REFUSED_STREAM();
    int nova_http2_constants_CANCEL();
    int nova_http2_constants_COMPRESSION_ERROR();
    int nova_http2_constants_CONNECT_ERROR();
    int nova_http2_constants_ENHANCE_YOUR_CALM();
    int nova_http2_constants_INADEQUATE_SECURITY();
    int nova_http2_constants_HTTP_1_1_REQUIRED();

    int nova_http2_constants_HEADER_TABLE_SIZE();
    int nova_http2_constants_ENABLE_PUSH();
    int nova_http2_constants_MAX_CONCURRENT_STREAMS();
    int nova_http2_constants_INITIAL_WINDOW_SIZE();
    int nova_http2_constants_MAX_FRAME_SIZE();
    int nova_http2_constants_MAX_HEADER_LIST_SIZE();
    int nova_http2_constants_ENABLE_CONNECT_PROTOCOL();

    int nova_http2_constants_DEFAULT_HEADER_TABLE_SIZE();
    int nova_http2_constants_DEFAULT_ENABLE_PUSH();
    int nova_http2_constants_DEFAULT_MAX_CONCURRENT_STREAMS();
    int nova_http2_constants_DEFAULT_INITIAL_WINDOW_SIZE();
    int nova_http2_constants_DEFAULT_MAX_FRAME_SIZE();
    int nova_http2_constants_DEFAULT_MAX_HEADER_LIST_SIZE();

    // ============================================================================
    // Settings Functions
    // ============================================================================

    void* nova_http2_getDefaultSettings();
    char* nova_http2_getPackedSettings(void* settingsPtr, int* length);
    void* nova_http2_getUnpackedSettings(const char* buffer, int length);
    void nova_http2_freeSettings(void* settingsPtr);

    // ============================================================================
    // Http2Session Functions
    // ============================================================================

    void* nova_http2_Session_new(int type);
    int nova_http2_Session_type(void* sessionPtr);
    int nova_http2_Session_destroyed(void* sessionPtr);
    int nova_http2_Session_closed(void* sessionPtr);
    int nova_http2_Session_connecting(void* sessionPtr);
    void nova_http2_Session_settings(void* sessionPtr, void* settingsPtr);
    int nova_http2_Session_localSettings(void* sessionPtr, int index);
    int nova_http2_Session_remoteSettings(void* sessionPtr, int index);
    void nova_http2_Session_ping(void* sessionPtr, void* callback);
    void nova_http2_Session_goaway(void* sessionPtr, int code, int lastStreamId);
    void nova_http2_Session_close(void* sessionPtr, void* callback);
    void nova_http2_Session_destroy(void* sessionPtr, int code);
    void nova_http2_Session_on(void* sessionPtr, const char* event, void* handler);
    void nova_http2_Session_free(void* sessionPtr);

    // ============================================================================
    // Http2Stream Functions
    // ============================================================================

    void* nova_http2_Stream_new(void* sessionPtr);
    int nova_http2_Stream_id(void* streamPtr);
    int nova_http2_Stream_state(void* streamPtr);
    int nova_http2_Stream_closed(void* streamPtr);
    int nova_http2_Stream_destroyed(void* streamPtr);
    int nova_http2_Stream_sentHeaders(void* streamPtr);
    int nova_http2_Stream_sentTrailers(void* streamPtr);
    void* nova_http2_Stream_session(void* streamPtr);
    void nova_http2_Stream_priority(void* streamPtr, int weight, int exclusive);
    void nova_http2_Stream_respond(void* streamPtr, int statusCode, const char** headers, int headerCount);
    int nova_http2_Stream_write(void* streamPtr, const char* data, int length);
    void nova_http2_Stream_end(void* streamPtr, const char* data, int length);
    void nova_http2_Stream_close(void* streamPtr, int code);
    void nova_http2_Stream_rstStream(void* streamPtr, int code);
    void nova_http2_Stream_on(void* streamPtr, const char* event, void* handler);
    void nova_http2_Stream_free(void* streamPtr);

    // ============================================================================
    // Http2Server Functions
    // ============================================================================

    void* nova_http2_createServer(void* requestHandler);
    void* nova_http2_createSecureServer(void* options, void* requestHandler);
    int nova_http2_Server_listen(void* serverPtr, int port, const char* hostname, void* callback);
    void nova_http2_Server_close(void* serverPtr, void* callback);
    int nova_http2_Server_listening(void* serverPtr);
    void nova_http2_Server_setTimeout(void* serverPtr, int ms, void* callback);
    void nova_http2_Server_on(void* serverPtr, const char* event, void* handler);
    void nova_http2_Server_free(void* serverPtr);

    // ============================================================================
    // Client Connect
    // ============================================================================

    void* nova_http2_connect(const char* authority, void* options, void* listener);
    void* nova_http2_ClientSession_request(void* sessionPtr, const char** headers, int headerCount);

    // ============================================================================
    // Http2ServerRequest Functions
    // ============================================================================

    void* nova_http2_ServerRequest_new(void* streamPtr);
    char* nova_http2_ServerRequest_method(void* reqPtr);
    char* nova_http2_ServerRequest_authority(void* reqPtr);
    char* nova_http2_ServerRequest_scheme(void* reqPtr);
    char* nova_http2_ServerRequest_path(void* reqPtr);
    char* nova_http2_ServerRequest_httpVersion(void* reqPtr);
    void* nova_http2_ServerRequest_stream(void* reqPtr);
    void nova_http2_ServerRequest_free(void* reqPtr);

    // ============================================================================
    // Http2ServerResponse Functions
    // ============================================================================

    void* nova_http2_ServerResponse_new(void* streamPtr);
    void nova_http2_ServerResponse_setStatusCode(void* resPtr, int code);
    int nova_http2_ServerResponse_statusCode(void* resPtr);
    void nova_http2_ServerResponse_setHeader(void* resPtr, const char* name, const char* value);
    int nova_http2_ServerResponse_write(void* resPtr, const char* data, int length);
    void nova_http2_ServerResponse_end(void* resPtr, const char* data, int length);
    int nova_http2_ServerResponse_finished(void* resPtr);
    void* nova_http2_ServerResponse_stream(void* resPtr);
    void nova_http2_ServerResponse_free(void* resPtr);

    // ============================================================================
    // Sensitive Headers Symbol
    // ============================================================================

    void* nova_http2_sensitiveHeaders();

    // ============================================================================
    // Utility Functions
    // ============================================================================

    void nova_http2_cleanup();
}

} // namespace http2

// ============================================================================
// Module Registry
// ============================================================================

// Check if module is a built-in nova module
bool isBuiltinModule(const std::string& modulePath);

// Get list of available built-in modules
std::vector<std::string> getBuiltinModules();

} // namespace runtime
} // namespace nova
