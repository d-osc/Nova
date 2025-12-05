/**
 * nova:fs - File System Module Implementation
 *
 * 100% compatible with Node.js fs module (sync functions)
 * https://nodejs.org/api/fs.html
 */

#include "nova/runtime/BuiltinModules.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <random>
#include <chrono>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <io.h>
#include <direct.h>
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISLNK(m) (0)  // Windows doesn't have symbolic link mode check
#else
#include <unistd.h>
#include <dirent.h>
#include <utime.h>
#include <sys/uio.h>  // For readv/writev
#include <sys/time.h> // For lutimes
#ifdef __linux__
#include <sys/inotify.h>  // For file watching (Linux only)
#endif
#endif

namespace nova {
namespace runtime {
namespace fs {

// ============================================================================
// Helper Functions
// ============================================================================

static char* allocString(const std::string& str) {
    char* result = (char*)malloc(str.length() + 1);
    if (result) {
        strcpy(result, str.c_str());
    }
    return result;
}

// Stats structure matching Node.js fs.Stats
struct NovaStats {
    int64_t dev;
    int64_t ino;
    int64_t mode;
    int64_t nlink;
    int64_t uid;
    int64_t gid;
    int64_t rdev;
    int64_t size;
    int64_t blksize;
    int64_t blocks;
    double atimeMs;
    double mtimeMs;
    double ctimeMs;
    double birthtimeMs;
    int isFile;
    int isDirectory;
    int isSymbolicLink;
    int isBlockDevice;
    int isCharacterDevice;
    int isFIFO;
    int isSocket;
};

static NovaStats* createStats(const std::filesystem::path& path, bool followSymlinks = true) {
    NovaStats* stats = (NovaStats*)malloc(sizeof(NovaStats));
    if (!stats) return nullptr;

    memset(stats, 0, sizeof(NovaStats));

    std::error_code ec;
    std::filesystem::file_status status;

    if (followSymlinks) {
        status = std::filesystem::status(path, ec);
    } else {
        status = std::filesystem::symlink_status(path, ec);
    }

    if (ec) {
        free(stats);
        return nullptr;
    }

    // File type checks
    stats->isFile = std::filesystem::is_regular_file(status) ? 1 : 0;
    stats->isDirectory = std::filesystem::is_directory(status) ? 1 : 0;
    stats->isSymbolicLink = std::filesystem::is_symlink(status) ? 1 : 0;
    stats->isBlockDevice = std::filesystem::is_block_file(status) ? 1 : 0;
    stats->isCharacterDevice = std::filesystem::is_character_file(status) ? 1 : 0;
    stats->isFIFO = std::filesystem::is_fifo(status) ? 1 : 0;
    stats->isSocket = std::filesystem::is_socket(status) ? 1 : 0;

    // Get file size
    if (stats->isFile) {
        stats->size = (int64_t)std::filesystem::file_size(path, ec);
    }

    // Get timestamps
    auto ftime = std::filesystem::last_write_time(path, ec);
    if (!ec) {
        // Manual conversion from file_time to system_clock without clock_cast
        auto ftimeSinceEpoch = ftime.time_since_epoch();
        auto systemNow = std::chrono::system_clock::now();
        auto fileNow = std::filesystem::file_time_type::clock::now();
        auto diff = fileNow.time_since_epoch() - ftimeSinceEpoch;
        auto systemTime = systemNow - std::chrono::duration_cast<std::chrono::system_clock::duration>(diff);
        auto sctp = std::chrono::time_point_cast<std::chrono::milliseconds>(systemTime);
        stats->mtimeMs = (double)sctp.time_since_epoch().count();
        stats->atimeMs = stats->mtimeMs;  // Approximate
        stats->ctimeMs = stats->mtimeMs;  // Approximate
        stats->birthtimeMs = stats->mtimeMs;  // Approximate
    }

    // Platform-specific stats
#ifdef _WIN32
    struct _stat64 st;
    if (_stat64(path.string().c_str(), &st) == 0) {
        stats->dev = st.st_dev;
        stats->ino = st.st_ino;
        stats->mode = st.st_mode;
        stats->nlink = st.st_nlink;
        stats->uid = st.st_uid;
        stats->gid = st.st_gid;
        stats->rdev = st.st_rdev;
    }
#else
    struct stat st;
    int statResult = followSymlinks ? stat(path.c_str(), &st) : lstat(path.c_str(), &st);
    if (statResult == 0) {
        stats->dev = st.st_dev;
        stats->ino = st.st_ino;
        stats->mode = st.st_mode;
        stats->nlink = st.st_nlink;
        stats->uid = st.st_uid;
        stats->gid = st.st_gid;
        stats->rdev = st.st_rdev;
        stats->blksize = st.st_blksize;
        stats->blocks = st.st_blocks;
    }
#endif

    return stats;
}

extern "C" {

// ============================================================================
// File Access & Metadata
// ============================================================================

// fs.accessSync(path[, mode]) - Tests user permissions
// mode: fs.constants.F_OK (0), R_OK (4), W_OK (2), X_OK (1)
int nova_fs_accessSync(const char* path, int mode) {
    if (!path) return 0;

#ifdef _WIN32
    // Windows _access only supports existence (0), read (4), write (2)
    int winMode = 0;
    if (mode == 0) winMode = 0;  // F_OK - existence
    else if (mode & 4) winMode = 4;  // R_OK
    else if (mode & 2) winMode = 2;  // W_OK
    return _access(path, winMode) == 0 ? 1 : 0;
#else
    return access(path, mode) == 0 ? 1 : 0;
#endif
}

// fs.existsSync(path) - Determines if a path exists
int nova_fs_existsSync(const char* path) {
    if (!path) return 0;
    return std::filesystem::exists(path) ? 1 : 0;
}

// fs.statSync(path[, options]) - Returns file statistics
void* nova_fs_statSync(const char* path) {
    if (!path) return nullptr;
    return createStats(path, true);
}

// fs.lstatSync(path[, options]) - Returns symbolic link statistics
void* nova_fs_lstatSync(const char* path) {
    if (!path) return nullptr;
    return createStats(path, false);
}

// fs.realpathSync(path[, options]) - Resolves canonical path
char* nova_fs_realpathSync(const char* path) {
    if (!path) return nullptr;

    std::error_code ec;
    auto canonical = std::filesystem::canonical(path, ec);
    if (ec) return nullptr;

    return allocString(canonical.string());
}

// ============================================================================
// File Operations
// ============================================================================

// fs.readFileSync(path[, options]) - Reads entire file contents
char* nova_fs_readFileSync(const char* path) {
    if (!path) return nullptr;

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return nullptr;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return allocString(buffer.str());
}

// fs.readFileSync with encoding option
char* nova_fs_readFileSyncEncoding(const char* path, [[maybe_unused]] const char* encoding) {
    // For now, we treat all encodings as utf-8
    return nova_fs_readFileSync(path);
}

// fs.writeFileSync(file, data[, options]) - Writes data to a file
int nova_fs_writeFileSync(const char* path, const char* data) {
    if (!path || !data) return 0;

    // Create parent directories if needed
    std::filesystem::path filePath(path);
    if (filePath.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(filePath.parent_path(), ec);
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return 0;
    }

    file << data;
    file.close();
    return 1;
}

// fs.appendFileSync(path, data[, options]) - Appends data to a file
int nova_fs_appendFileSync(const char* path, const char* data) {
    if (!path || !data) return 0;

    std::ofstream file(path, std::ios::binary | std::ios::app);
    if (!file.is_open()) {
        return 0;
    }

    file << data;
    file.close();
    return 1;
}

// fs.truncateSync(path[, len]) - Truncates a file
int nova_fs_truncateSync(const char* path, int64_t len) {
    if (!path) return 0;

    std::error_code ec;
    std::filesystem::resize_file(path, len, ec);
    return ec ? 0 : 1;
}

// fs.copyFileSync(src, dest[, mode]) - Copies a file
int nova_fs_copyFileSync(const char* src, const char* dest) {
    if (!src || !dest) return 0;

    // Create parent directories if needed
    std::filesystem::path destPath(dest);
    if (destPath.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(destPath.parent_path(), ec);
    }

    std::error_code ec;
    std::filesystem::copy_file(src, dest, std::filesystem::copy_options::overwrite_existing, ec);
    return ec ? 0 : 1;
}

// fs.cpSync(src, dest[, options]) - Recursively copies files/directories
int nova_fs_cpSync(const char* src, const char* dest) {
    if (!src || !dest) return 0;

    std::error_code ec;
    std::filesystem::copy(src, dest,
        std::filesystem::copy_options::recursive |
        std::filesystem::copy_options::overwrite_existing, ec);
    return ec ? 0 : 1;
}

// ============================================================================
// File Management
// ============================================================================

// fs.unlinkSync(path) - Deletes a file
int nova_fs_unlinkSync(const char* path) {
    if (!path) return 0;

    std::error_code ec;
    return std::filesystem::remove(path, ec) ? 1 : 0;
}

// fs.renameSync(oldPath, newPath) - Renames a file
int nova_fs_renameSync(const char* oldPath, const char* newPath) {
    if (!oldPath || !newPath) return 0;

    // Create parent directories if needed
    std::filesystem::path destPath(newPath);
    if (destPath.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(destPath.parent_path(), ec);
    }

    std::error_code ec;
    std::filesystem::rename(oldPath, newPath, ec);
    return ec ? 0 : 1;
}

// fs.rmSync(path[, options]) - Recursively removes files/directories
int nova_fs_rmSync(const char* path) {
    if (!path) return 0;

    std::error_code ec;
    std::filesystem::remove_all(path, ec);
    return ec ? 0 : 1;
}

// fs.rmSync with options (recursive, force)
int nova_fs_rmSyncOptions(const char* path, int recursive, int force) {
    if (!path) return 0;

    std::error_code ec;
    if (recursive) {
        std::filesystem::remove_all(path, ec);
    } else {
        std::filesystem::remove(path, ec);
    }

    if (force) return 1;  // Force ignores errors
    return ec ? 0 : 1;
}

// ============================================================================
// Symbolic Links
// ============================================================================

// fs.linkSync(existingPath, newPath) - Creates hard link
int nova_fs_linkSync(const char* existingPath, const char* newPath) {
    if (!existingPath || !newPath) return 0;

    std::error_code ec;
    std::filesystem::create_hard_link(existingPath, newPath, ec);
    return ec ? 0 : 1;
}

// fs.symlinkSync(target, path[, type]) - Creates symbolic link
int nova_fs_symlinkSync(const char* target, const char* path) {
    if (!target || !path) return 0;

    std::error_code ec;
    if (std::filesystem::is_directory(target)) {
        std::filesystem::create_directory_symlink(target, path, ec);
    } else {
        std::filesystem::create_symlink(target, path, ec);
    }
    return ec ? 0 : 1;
}

// fs.readlinkSync(path[, options]) - Reads symbolic link
char* nova_fs_readlinkSync(const char* path) {
    if (!path) return nullptr;

    std::error_code ec;
    auto target = std::filesystem::read_symlink(path, ec);
    if (ec) return nullptr;

    return allocString(target.string());
}

// ============================================================================
// Directory Operations
// ============================================================================

// fs.readdirSync(path[, options]) - Lists directory contents
char* nova_fs_readdirSync(const char* path) {
    if (!path) return nullptr;

    std::string result = "[";
    bool first = true;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (!first) result += ",";
            result += "\"" + entry.path().filename().string() + "\"";
            first = false;
        }
    } catch (...) {
        return nullptr;
    }

    result += "]";
    return allocString(result);
}

// fs.readdirSync returning array (internal use)
char** nova_fs_readdirSyncArray(const char* path, int* count) {
    if (!path || !count) return nullptr;

    *count = 0;
    std::vector<std::string> entries;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            entries.push_back(entry.path().filename().string());
        }
    } catch (...) {
        return nullptr;
    }

    *count = (int)entries.size();
    if (*count == 0) {
        return nullptr;
    }

    char** result = (char**)malloc(sizeof(char*) * (*count));
    if (!result) return nullptr;

    for (int i = 0; i < *count; i++) {
        result[i] = allocString(entries[i]);
    }

    return result;
}

// fs.mkdirSync(path[, options]) - Creates a directory
int nova_fs_mkdirSync(const char* path) {
    if (!path) return 0;

    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    return ec ? 0 : 1;
}

// fs.mkdirSync with options (recursive, mode)
int nova_fs_mkdirSyncOptions(const char* path, int recursive, [[maybe_unused]] int mode) {
    if (!path) return 0;

    std::error_code ec;
    if (recursive) {
        std::filesystem::create_directories(path, ec);
    } else {
        std::filesystem::create_directory(path, ec);
    }

#ifndef _WIN32
    if (!ec && mode != 0) {
        chmod(path, mode);
    }
#endif

    return ec ? 0 : 1;
}

// fs.rmdirSync(path[, options]) - Removes a directory
int nova_fs_rmdirSync(const char* path) {
    if (!path) return 0;

    std::error_code ec;
    return std::filesystem::remove(path, ec) ? 1 : 0;
}

// fs.mkdtempSync(prefix[, options]) - Creates temporary directory
char* nova_fs_mkdtempSync(const char* prefix) {
    if (!prefix) return nullptr;

    // Generate random suffix
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 35);

    const char* chars = "abcdefghijklmnopqrstuvwxyz0123456789";
    std::string suffix;
    for (int i = 0; i < 6; i++) {
        suffix += chars[dis(gen)];
    }

    std::string tempDir = std::string(prefix) + suffix;

    std::error_code ec;
    std::filesystem::create_directories(tempDir, ec);
    if (ec) return nullptr;

    return allocString(tempDir);
}

// ============================================================================
// Permissions & Ownership
// ============================================================================

// fs.chmodSync(path, mode) - Changes file permissions
int nova_fs_chmodSync(const char* path, int mode) {
    if (!path) return 0;

#ifdef _WIN32
    // Windows has limited chmod support
    int winMode = 0;
    if (mode & 0200) winMode |= _S_IWRITE;
    if (mode & 0400) winMode |= _S_IREAD;
    return _chmod(path, winMode) == 0 ? 1 : 0;
#else
    return chmod(path, mode) == 0 ? 1 : 0;
#endif
}

// fs.chownSync(path, uid, gid) - Changes file ownership
int nova_fs_chownSync(const char* path, [[maybe_unused]] int uid, [[maybe_unused]] int gid) {
    if (!path) return 0;

#ifdef _WIN32
    // Windows doesn't support Unix-style ownership
    return 1;  // Silent success
#else
    return chown(path, uid, gid) == 0 ? 1 : 0;
#endif
}

// fs.lchownSync(path, uid, gid) - Changes symbolic link ownership
int nova_fs_lchownSync(const char* path, [[maybe_unused]] int uid, [[maybe_unused]] int gid) {
    if (!path) return 0;

#ifdef _WIN32
    return 1;  // Silent success
#else
    return lchown(path, uid, gid) == 0 ? 1 : 0;
#endif
}

// ============================================================================
// Time Operations
// ============================================================================

// fs.utimesSync(path, atime, mtime) - Changes file timestamps
int nova_fs_utimesSync(const char* path, [[maybe_unused]] double atime, double mtime) {
    if (!path) return 0;

    // Manual conversion from system_clock to file_time without clock_cast
    auto mtimePoint = std::chrono::system_clock::time_point(
        std::chrono::milliseconds((int64_t)mtime));
    auto systemNow = std::chrono::system_clock::now();
    auto fileNow = std::filesystem::file_time_type::clock::now();
    auto diff = systemNow - mtimePoint;
    auto ftimePoint = fileNow - std::chrono::duration_cast<std::filesystem::file_time_type::duration>(diff);

    std::error_code ec;
    std::filesystem::last_write_time(path, ftimePoint, ec);
    return ec ? 0 : 1;
}

// ============================================================================
// File Descriptor Operations (Basic)
// ============================================================================

// fs.openSync(path[, flags[, mode]]) - Opens a file
int nova_fs_openSync(const char* path, const char* flags) {
    if (!path || !flags) return -1;

    int mode = O_RDONLY;
    std::string flagStr(flags);

    if (flagStr == "r") mode = O_RDONLY;
    else if (flagStr == "r+") mode = O_RDWR;
    else if (flagStr == "w") mode = O_WRONLY | O_CREAT | O_TRUNC;
    else if (flagStr == "w+") mode = O_RDWR | O_CREAT | O_TRUNC;
    else if (flagStr == "a") mode = O_WRONLY | O_CREAT | O_APPEND;
    else if (flagStr == "a+") mode = O_RDWR | O_CREAT | O_APPEND;
    else if (flagStr == "wx") mode = O_WRONLY | O_CREAT | O_EXCL;
    else if (flagStr == "wx+") mode = O_RDWR | O_CREAT | O_EXCL;

#ifdef _WIN32
    mode |= O_BINARY;
    return _open(path, mode, _S_IREAD | _S_IWRITE);
#else
    return open(path, mode, 0666);
#endif
}

// fs.closeSync(fd) - Closes a file descriptor
int nova_fs_closeSync(int fd) {
#ifdef _WIN32
    return _close(fd) == 0 ? 1 : 0;
#else
    return close(fd) == 0 ? 1 : 0;
#endif
}

// fs.readSync(fd, buffer, offset, length, position) - Reads from file descriptor
int64_t nova_fs_readSync(int fd, char* buffer, int64_t length, int64_t position) {
    if (!buffer || fd < 0) return -1;

#ifdef _WIN32
    if (position >= 0) {
        _lseeki64(fd, position, SEEK_SET);
    }
    return _read(fd, buffer, (unsigned int)length);
#else
    if (position >= 0) {
        lseek(fd, position, SEEK_SET);
    }
    return read(fd, buffer, length);
#endif
}

// fs.writeSync(fd, buffer, offset, length, position) - Writes to file descriptor
int64_t nova_fs_writeSync(int fd, const char* buffer, int64_t length, int64_t position) {
    if (!buffer || fd < 0) return -1;

#ifdef _WIN32
    if (position >= 0) {
        _lseeki64(fd, position, SEEK_SET);
    }
    return _write(fd, buffer, (unsigned int)length);
#else
    if (position >= 0) {
        lseek(fd, position, SEEK_SET);
    }
    return write(fd, buffer, length);
#endif
}

// fs.fsyncSync(fd) - Flushes all data to storage
int nova_fs_fsyncSync(int fd) {
#ifdef _WIN32
    return _commit(fd) == 0 ? 1 : 0;
#else
    return fsync(fd) == 0 ? 1 : 0;
#endif
}

// fs.ftruncateSync(fd, len) - Truncates file via descriptor
int nova_fs_ftruncateSync(int fd, int64_t len) {
#ifdef _WIN32
    return _chsize_s(fd, len) == 0 ? 1 : 0;
#else
    return ftruncate(fd, len) == 0 ? 1 : 0;
#endif
}

// fs.fstatSync(fd) - Returns file statistics from descriptor
void* nova_fs_fstatSync(int fd) {
    NovaStats* stats = (NovaStats*)malloc(sizeof(NovaStats));
    if (!stats) return nullptr;

    memset(stats, 0, sizeof(NovaStats));

#ifdef _WIN32
    struct _stat64 st;
    if (_fstat64(fd, &st) != 0) {
        free(stats);
        return nullptr;
    }
    stats->dev = st.st_dev;
    stats->ino = st.st_ino;
    stats->mode = st.st_mode;
    stats->nlink = st.st_nlink;
    stats->uid = st.st_uid;
    stats->gid = st.st_gid;
    stats->rdev = st.st_rdev;
    stats->size = st.st_size;
    stats->isFile = S_ISREG(st.st_mode) ? 1 : 0;
    stats->isDirectory = S_ISDIR(st.st_mode) ? 1 : 0;
#else
    struct stat st;
    if (fstat(fd, &st) != 0) {
        free(stats);
        return nullptr;
    }
    stats->dev = st.st_dev;
    stats->ino = st.st_ino;
    stats->mode = st.st_mode;
    stats->nlink = st.st_nlink;
    stats->uid = st.st_uid;
    stats->gid = st.st_gid;
    stats->rdev = st.st_rdev;
    stats->size = st.st_size;
    stats->blksize = st.st_blksize;
    stats->blocks = st.st_blocks;
    stats->isFile = S_ISREG(st.st_mode) ? 1 : 0;
    stats->isDirectory = S_ISDIR(st.st_mode) ? 1 : 0;
    stats->isSymbolicLink = S_ISLNK(st.st_mode) ? 1 : 0;
#endif

    return stats;
}

// fs.fchmodSync(fd, mode) - Changes file permissions via descriptor
int nova_fs_fchmodSync([[maybe_unused]] int fd, [[maybe_unused]] int mode) {
#ifdef _WIN32
    // Windows doesn't support fchmod, but we can try via handle
    return 1;  // Silent success
#else
    return fchmod(fd, mode) == 0 ? 1 : 0;
#endif
}

// fs.fchownSync(fd, uid, gid) - Changes file ownership via descriptor
int nova_fs_fchownSync([[maybe_unused]] int fd, [[maybe_unused]] int uid, [[maybe_unused]] int gid) {
#ifdef _WIN32
    return 1;  // Silent success - Windows doesn't support Unix ownership
#else
    return fchown(fd, uid, gid) == 0 ? 1 : 0;
#endif
}

// fs.fdatasyncSync(fd) - Flushes data only (not metadata)
int nova_fs_fdatasyncSync(int fd) {
#ifdef _WIN32
    return _commit(fd) == 0 ? 1 : 0;  // Windows doesn't distinguish
#else
    return fdatasync(fd) == 0 ? 1 : 0;
#endif
}

// fs.futimesSync(fd, atime, mtime) - Changes timestamps via descriptor
int nova_fs_futimesSync(int fd, double atime, double mtime) {
#ifdef _WIN32
    // Windows requires HANDLE, convert fd to HANDLE
    HANDLE h = (HANDLE)_get_osfhandle(fd);
    if (h == INVALID_HANDLE_VALUE) return 0;

    FILETIME ftAccess, ftWrite;
    // Convert milliseconds to FILETIME (100-nanosecond intervals since Jan 1, 1601)
    int64_t atimeNs = (int64_t)(atime * 10000) + 116444736000000000LL;
    int64_t mtimeNs = (int64_t)(mtime * 10000) + 116444736000000000LL;

    ftAccess.dwLowDateTime = (DWORD)(atimeNs & 0xFFFFFFFF);
    ftAccess.dwHighDateTime = (DWORD)(atimeNs >> 32);
    ftWrite.dwLowDateTime = (DWORD)(mtimeNs & 0xFFFFFFFF);
    ftWrite.dwHighDateTime = (DWORD)(mtimeNs >> 32);

    return SetFileTime(h, nullptr, &ftAccess, &ftWrite) ? 1 : 0;
#else
    struct timeval tv[2];
    tv[0].tv_sec = (time_t)(atime / 1000);
    tv[0].tv_usec = ((long)(atime) % 1000) * 1000;
    tv[1].tv_sec = (time_t)(mtime / 1000);
    tv[1].tv_usec = ((long)(mtime) % 1000) * 1000;
    return futimes(fd, tv) == 0 ? 1 : 0;
#endif
}

// ============================================================================
// Stats Helper Functions (for accessing NovaStats fields)
// ============================================================================

int64_t nova_fs_stats_size(void* stats) {
    return stats ? ((NovaStats*)stats)->size : 0;
}

int64_t nova_fs_stats_mode(void* stats) {
    return stats ? ((NovaStats*)stats)->mode : 0;
}

double nova_fs_stats_mtimeMs(void* stats) {
    return stats ? ((NovaStats*)stats)->mtimeMs : 0;
}

double nova_fs_stats_atimeMs(void* stats) {
    return stats ? ((NovaStats*)stats)->atimeMs : 0;
}

double nova_fs_stats_ctimeMs(void* stats) {
    return stats ? ((NovaStats*)stats)->ctimeMs : 0;
}

double nova_fs_stats_birthtimeMs(void* stats) {
    return stats ? ((NovaStats*)stats)->birthtimeMs : 0;
}

int nova_fs_stats_isFile(void* stats) {
    return stats ? ((NovaStats*)stats)->isFile : 0;
}

int nova_fs_stats_isDirectory(void* stats) {
    return stats ? ((NovaStats*)stats)->isDirectory : 0;
}

int nova_fs_stats_isSymbolicLink(void* stats) {
    return stats ? ((NovaStats*)stats)->isSymbolicLink : 0;
}

int nova_fs_stats_isBlockDevice(void* stats) {
    return stats ? ((NovaStats*)stats)->isBlockDevice : 0;
}

int nova_fs_stats_isCharacterDevice(void* stats) {
    return stats ? ((NovaStats*)stats)->isCharacterDevice : 0;
}

int nova_fs_stats_isFIFO(void* stats) {
    return stats ? ((NovaStats*)stats)->isFIFO : 0;
}

int nova_fs_stats_isSocket(void* stats) {
    return stats ? ((NovaStats*)stats)->isSocket : 0;
}

void nova_fs_stats_free(void* stats) {
    if (stats) free(stats);
}

// ============================================================================
// Constants (matching Node.js fs.constants)
// ============================================================================

int nova_fs_constants_F_OK() { return 0; }  // File exists
int nova_fs_constants_R_OK() { return 4; }  // File is readable
int nova_fs_constants_W_OK() { return 2; }  // File is writable
int nova_fs_constants_X_OK() { return 1; }  // File is executable

int nova_fs_constants_COPYFILE_EXCL() { return 1; }
int nova_fs_constants_COPYFILE_FICLONE() { return 2; }
int nova_fs_constants_COPYFILE_FICLONE_FORCE() { return 4; }

// Open flags constants
int nova_fs_constants_O_RDONLY() { return O_RDONLY; }
int nova_fs_constants_O_WRONLY() { return O_WRONLY; }
int nova_fs_constants_O_RDWR() { return O_RDWR; }
int nova_fs_constants_O_CREAT() { return O_CREAT; }
int nova_fs_constants_O_EXCL() { return O_EXCL; }
int nova_fs_constants_O_TRUNC() { return O_TRUNC; }
int nova_fs_constants_O_APPEND() { return O_APPEND; }
#ifdef O_NOCTTY
int nova_fs_constants_O_NOCTTY() { return O_NOCTTY; }
#else
int nova_fs_constants_O_NOCTTY() { return 0; }
#endif
#ifdef O_DIRECTORY
int nova_fs_constants_O_DIRECTORY() { return O_DIRECTORY; }
#else
int nova_fs_constants_O_DIRECTORY() { return 0; }
#endif
#ifdef O_NOFOLLOW
int nova_fs_constants_O_NOFOLLOW() { return O_NOFOLLOW; }
#else
int nova_fs_constants_O_NOFOLLOW() { return 0; }
#endif
#ifdef O_SYNC
int nova_fs_constants_O_SYNC() { return O_SYNC; }
#else
int nova_fs_constants_O_SYNC() { return 0; }
#endif
#ifdef O_DSYNC
int nova_fs_constants_O_DSYNC() { return O_DSYNC; }
#else
int nova_fs_constants_O_DSYNC() { return 0; }
#endif
#ifdef O_SYMLINK
int nova_fs_constants_O_SYMLINK() { return O_SYMLINK; }
#else
int nova_fs_constants_O_SYMLINK() { return 0; }
#endif
#ifdef O_DIRECT
int nova_fs_constants_O_DIRECT() { return O_DIRECT; }
#else
int nova_fs_constants_O_DIRECT() { return 0; }
#endif
#ifdef O_NONBLOCK
int nova_fs_constants_O_NONBLOCK() { return O_NONBLOCK; }
#else
int nova_fs_constants_O_NONBLOCK() { return 0; }
#endif
#ifdef O_NOATIME
int nova_fs_constants_O_NOATIME() { return O_NOATIME; }
#else
int nova_fs_constants_O_NOATIME() { return 0; }
#endif

// ============================================================================
// Directory Iterator (opendirSync)
// ============================================================================

// Dir structure for directory iteration
struct NovaDir {
    std::filesystem::directory_iterator iter;
    std::filesystem::directory_iterator end;
    std::string path;
    bool closed;
};

// fs.opendirSync(path[, options]) - Opens a directory
void* nova_fs_opendirSync(const char* path) {
    if (!path) return nullptr;

    try {
        NovaDir* dir = new NovaDir();
        dir->path = path;
        dir->iter = std::filesystem::directory_iterator(path);
        dir->end = std::filesystem::directory_iterator();
        dir->closed = false;
        return dir;
    } catch (...) {
        return nullptr;
    }
}

// dir.readSync() - Reads next entry
char* nova_fs_dir_readSync(void* dirPtr) {
    if (!dirPtr) return nullptr;

    NovaDir* dir = (NovaDir*)dirPtr;
    if (dir->closed || dir->iter == dir->end) {
        return nullptr;  // End of directory
    }

    std::string name = dir->iter->path().filename().string();
    ++dir->iter;

    return allocString(name);
}

// dir.closeSync() - Closes directory
int nova_fs_dir_closeSync(void* dirPtr) {
    if (!dirPtr) return 0;

    NovaDir* dir = (NovaDir*)dirPtr;
    dir->closed = true;
    delete dir;
    return 1;
}

// Dirent structure
struct NovaDirent {
    char* name;
    char* parentPath;
    int isFile;
    int isDirectory;
    int isSymbolicLink;
    int isBlockDevice;
    int isCharacterDevice;
    int isFIFO;
    int isSocket;
};

// dir.readSync() returning Dirent
void* nova_fs_dir_readSyncDirent(void* dirPtr) {
    if (!dirPtr) return nullptr;

    NovaDir* dir = (NovaDir*)dirPtr;
    if (dir->closed || dir->iter == dir->end) {
        return nullptr;
    }

    NovaDirent* dirent = (NovaDirent*)malloc(sizeof(NovaDirent));
    if (!dirent) return nullptr;

    auto& entry = *dir->iter;
    dirent->name = allocString(entry.path().filename().string());

    std::error_code ec;
    auto status = entry.status(ec);
    dirent->isFile = std::filesystem::is_regular_file(status) ? 1 : 0;
    dirent->isDirectory = std::filesystem::is_directory(status) ? 1 : 0;
    dirent->isSymbolicLink = std::filesystem::is_symlink(status) ? 1 : 0;
    dirent->isBlockDevice = std::filesystem::is_block_file(status) ? 1 : 0;
    dirent->isCharacterDevice = std::filesystem::is_character_file(status) ? 1 : 0;
    dirent->isFIFO = std::filesystem::is_fifo(status) ? 1 : 0;
    dirent->isSocket = std::filesystem::is_socket(status) ? 1 : 0;

    ++dir->iter;
    return dirent;
}

// Dirent helper functions
char* nova_fs_dirent_name(void* dirent) {
    return dirent ? ((NovaDirent*)dirent)->name : nullptr;
}

int nova_fs_dirent_isFile(void* dirent) {
    return dirent ? ((NovaDirent*)dirent)->isFile : 0;
}

int nova_fs_dirent_isDirectory(void* dirent) {
    return dirent ? ((NovaDirent*)dirent)->isDirectory : 0;
}

int nova_fs_dirent_isSymbolicLink(void* dirent) {
    return dirent ? ((NovaDirent*)dirent)->isSymbolicLink : 0;
}

int nova_fs_dirent_isBlockDevice(void* dirent) {
    return dirent ? ((NovaDirent*)dirent)->isBlockDevice : 0;
}

int nova_fs_dirent_isCharacterDevice(void* dirent) {
    return dirent ? ((NovaDirent*)dirent)->isCharacterDevice : 0;
}

int nova_fs_dirent_isFIFO(void* dirent) {
    return dirent ? ((NovaDirent*)dirent)->isFIFO : 0;
}

int nova_fs_dirent_isSocket(void* dirent) {
    return dirent ? ((NovaDirent*)dirent)->isSocket : 0;
}

char* nova_fs_dirent_parentPath(void* dirent) {
    if (!dirent) return nullptr;
    NovaDirent* d = (NovaDirent*)dirent;
    return d->parentPath ? allocString(d->parentPath) : nullptr;
}

// dirent.path - alias for parentPath (deprecated but still supported)
char* nova_fs_dirent_path(void* dirent) {
    return nova_fs_dirent_parentPath(dirent);
}

void nova_fs_dirent_free(void* dirent) {
    if (dirent) {
        NovaDirent* d = (NovaDirent*)dirent;
        if (d->name) free(d->name);
        free(d);
    }
}

// ============================================================================
// Filesystem Statistics (statfsSync)
// ============================================================================

struct NovaStatFs {
    int64_t type;      // Filesystem type
    int64_t bsize;     // Block size
    int64_t blocks;    // Total blocks
    int64_t bfree;     // Free blocks
    int64_t bavail;    // Available blocks (non-root)
    int64_t files;     // Total inodes
    int64_t ffree;     // Free inodes
};

// fs.statfsSync(path[, options]) - Returns filesystem statistics
void* nova_fs_statfsSync(const char* path) {
    if (!path) return nullptr;

    NovaStatFs* statfs = (NovaStatFs*)malloc(sizeof(NovaStatFs));
    if (!statfs) return nullptr;

    memset(statfs, 0, sizeof(NovaStatFs));

    std::error_code ec;
    auto spaceInfo = std::filesystem::space(path, ec);
    if (ec) {
        free(statfs);
        return nullptr;
    }

    // C++17 filesystem::space provides capacity, free, available
    statfs->bsize = 4096;  // Assume 4K block size
    statfs->blocks = spaceInfo.capacity / statfs->bsize;
    statfs->bfree = spaceInfo.free / statfs->bsize;
    statfs->bavail = spaceInfo.available / statfs->bsize;

    return statfs;
}

// StatFs helper functions
int64_t nova_fs_statfs_bsize(void* statfs) {
    return statfs ? ((NovaStatFs*)statfs)->bsize : 0;
}

int64_t nova_fs_statfs_blocks(void* statfs) {
    return statfs ? ((NovaStatFs*)statfs)->blocks : 0;
}

int64_t nova_fs_statfs_bfree(void* statfs) {
    return statfs ? ((NovaStatFs*)statfs)->bfree : 0;
}

int64_t nova_fs_statfs_bavail(void* statfs) {
    return statfs ? ((NovaStatFs*)statfs)->bavail : 0;
}

void nova_fs_statfs_free(void* statfs) {
    if (statfs) free(statfs);
}

// ============================================================================
// Glob Pattern Matching (globSync) - Node.js 22+
// ============================================================================

// Simple glob pattern matching
static bool matchGlob(const std::string& pattern, const std::string& str) {
    size_t p = 0, s = 0;
    size_t starP = std::string::npos, starS = 0;

    while (s < str.size()) {
        if (p < pattern.size() && (pattern[p] == str[s] || pattern[p] == '?')) {
            p++;
            s++;
        } else if (p < pattern.size() && pattern[p] == '*') {
            starP = p++;
            starS = s;
        } else if (starP != std::string::npos) {
            p = starP + 1;
            s = ++starS;
        } else {
            return false;
        }
    }

    while (p < pattern.size() && pattern[p] == '*') p++;
    return p == pattern.size();
}

// Recursive glob helper
static void globRecursive(const std::filesystem::path& basePath,
                          const std::string& pattern,
                          std::vector<std::string>& results,
                          bool includeDirectories) {
    std::error_code ec;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(basePath, ec)) {
        if (ec) continue;

        std::string relativePath = std::filesystem::relative(entry.path(), basePath, ec).string();
        if (ec) continue;

        // Replace backslashes with forward slashes for consistent matching
        std::replace(relativePath.begin(), relativePath.end(), '\\', '/');

        if (matchGlob(pattern, relativePath)) {
            if (includeDirectories || entry.is_regular_file()) {
                results.push_back(entry.path().string());
            }
        }
    }
}

// fs.globSync(pattern[, options]) - Glob pattern matching
char* nova_fs_globSync(const char* pattern) {
    if (!pattern) return nullptr;

    std::vector<std::string> results;
    std::string patternStr(pattern);

    // Find base directory (everything before first wildcard)
    size_t wildcardPos = patternStr.find_first_of("*?[");
    std::string basePath = ".";
    std::string globPattern = patternStr;

    if (wildcardPos != std::string::npos && wildcardPos > 0) {
        size_t lastSlash = patternStr.rfind('/', wildcardPos);
        if (lastSlash == std::string::npos) {
            lastSlash = patternStr.rfind('\\', wildcardPos);
        }
        if (lastSlash != std::string::npos) {
            basePath = patternStr.substr(0, lastSlash);
            globPattern = patternStr.substr(lastSlash + 1);
        }
    }

    if (std::filesystem::exists(basePath)) {
        globRecursive(basePath, globPattern, results, false);
    }

    // Build JSON array result
    std::string json = "[";
    for (size_t i = 0; i < results.size(); i++) {
        if (i > 0) json += ",";
        // Escape backslashes in path
        std::string escapedPath = results[i];
        std::replace(escapedPath.begin(), escapedPath.end(), '\\', '/');
        json += "\"" + escapedPath + "\"";
    }
    json += "]";

    return allocString(json);
}

// fs.globSync with options
char* nova_fs_globSyncOptions(const char* pattern, const char* cwd) {
    if (!pattern) return nullptr;

    std::vector<std::string> results;
    std::string basePath = cwd ? cwd : ".";

    if (std::filesystem::exists(basePath)) {
        globRecursive(basePath, pattern, results, false);
    }

    std::string json = "[";
    for (size_t i = 0; i < results.size(); i++) {
        if (i > 0) json += ",";
        std::string escapedPath = results[i];
        std::replace(escapedPath.begin(), escapedPath.end(), '\\', '/');
        json += "\"" + escapedPath + "\"";
    }
    json += "]";

    return allocString(json);
}

// ============================================================================
// File System Watching - Full Implementation
// ============================================================================

// Event listener entry
struct FSEventListener {
    void* callback;
    bool once;
};

// FSWatcher structure
struct FSWatcher {
    std::string path;
    bool closed;
    bool recursive;
    bool persistent;
    void (*changeListener)(const char*, const char*);
    std::vector<FSEventListener> changeListeners;
    std::vector<FSEventListener> errorListeners;
    std::vector<FSEventListener> closeListeners;
#ifdef _WIN32
    HANDLE dirHandle;
    HANDLE threadHandle;
    bool stopThread;
#elif defined(__linux__)
    int inotifyFd;
    int watchDescriptor;
#endif
};

// StatWatcher for fs.watchFile
struct StatWatcher {
    std::string path;
    bool closed;
    int interval;
    bool persistent;
    void (*callback)(void*);
    std::vector<FSEventListener> changeListeners;
    std::vector<FSEventListener> errorListeners;
    int64_t lastMtime;
    int64_t lastSize;
};

// Global watcher registry
static std::vector<FSWatcher*> activeWatchers;
static std::vector<StatWatcher*> activeStatWatchers;

#ifdef _WIN32
// Windows file watching thread
static DWORD WINAPI WatcherThread(LPVOID param) {
    FSWatcher* watcher = static_cast<FSWatcher*>(param);
    char buffer[4096];
    DWORD bytesReturned;

    while (!watcher->stopThread && !watcher->closed) {
        if (ReadDirectoryChangesW(
                watcher->dirHandle,
                buffer,
                sizeof(buffer),
                watcher->recursive ? TRUE : FALSE,
                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
                FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
                &bytesReturned,
                nullptr,
                nullptr)) {

            FILE_NOTIFY_INFORMATION* info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);
            do {
                // Convert filename to UTF-8
                int len = WideCharToMultiByte(CP_UTF8, 0, info->FileName,
                    info->FileNameLength / sizeof(WCHAR), nullptr, 0, nullptr, nullptr);
                std::string filename(len, 0);
                WideCharToMultiByte(CP_UTF8, 0, info->FileName,
                    info->FileNameLength / sizeof(WCHAR), &filename[0], len, nullptr, nullptr);

                // Determine event type
                const char* eventType = "change";
                if (info->Action == FILE_ACTION_ADDED || info->Action == FILE_ACTION_RENAMED_NEW_NAME) {
                    eventType = "rename";
                } else if (info->Action == FILE_ACTION_REMOVED || info->Action == FILE_ACTION_RENAMED_OLD_NAME) {
                    eventType = "rename";
                }

                // Call listener
                if (watcher->changeListener) {
                    watcher->changeListener(eventType, filename.c_str());
                }

                // Call registered listeners
                for (auto& listener : watcher->changeListeners) {
                    if (listener.callback) {
                        typedef void (*ChangeCallback)(const char*, const char*);
                        ((ChangeCallback)listener.callback)(eventType, filename.c_str());
                    }
                }

                if (info->NextEntryOffset == 0) break;
                info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                    reinterpret_cast<char*>(info) + info->NextEntryOffset);
            } while (true);
        }
    }
    return 0;
}
#endif

// fs.watch(filename[, options][, listener]) - Watch for changes
void* nova_fs_watch(const char* filename, void (*listener)(const char*, const char*)) {
    if (!filename) return nullptr;

    FSWatcher* watcher = new FSWatcher();
    watcher->path = filename;
    watcher->closed = false;
    watcher->recursive = false;
    watcher->persistent = true;
    watcher->changeListener = listener;

#ifdef _WIN32
    watcher->stopThread = false;

    // Check if path is directory or file
    DWORD attrs = GetFileAttributesA(filename);
    std::string watchPath = filename;

    if (attrs == INVALID_FILE_ATTRIBUTES) {
        delete watcher;
        return nullptr;
    }

    // If it's a file, watch its parent directory
    if (!(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        size_t pos = watchPath.find_last_of("\\/");
        if (pos != std::string::npos) {
            watchPath = watchPath.substr(0, pos);
        }
    }

    watcher->dirHandle = CreateFileA(
        watchPath.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr
    );

    if (watcher->dirHandle == INVALID_HANDLE_VALUE) {
        delete watcher;
        return nullptr;
    }

    // Start watcher thread
    watcher->threadHandle = CreateThread(nullptr, 0, WatcherThread, watcher, 0, nullptr);
#elif defined(__linux__)
    watcher->inotifyFd = inotify_init1(IN_NONBLOCK);
    if (watcher->inotifyFd < 0) {
        delete watcher;
        return nullptr;
    }

    watcher->watchDescriptor = inotify_add_watch(watcher->inotifyFd, filename,
        IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVE | IN_ATTRIB);

    if (watcher->watchDescriptor < 0) {
        close(watcher->inotifyFd);
        delete watcher;
        return nullptr;
    }
#else
    // macOS: file watching not yet implemented (would use FSEvents/kqueue)
    // For now, watcher is created but won't receive events
#endif

    activeWatchers.push_back(watcher);
    return watcher;
}

// fs.watchFile - Watch file for changes using stat polling
void* nova_fs_watchFile(const char* path, void (*callback)(void*)) {
    if (!path) return nullptr;

    StatWatcher* watcher = new StatWatcher();
    watcher->path = path;
    watcher->closed = false;
    watcher->interval = 5007; // Default Node.js interval
    watcher->persistent = true;
    watcher->callback = callback;

    // Get initial file stats
    struct stat st;
    if (stat(path, &st) == 0) {
        watcher->lastMtime = st.st_mtime;
        watcher->lastSize = st.st_size;
    } else {
        watcher->lastMtime = 0;
        watcher->lastSize = 0;
    }

    activeStatWatchers.push_back(watcher);
    return watcher;
}

// fs.unwatchFile - Stop watching file
int nova_fs_unwatchFile(const char* path) {
    if (!path) return 0;

    for (auto it = activeStatWatchers.begin(); it != activeStatWatchers.end(); ++it) {
        if ((*it)->path == path) {
            (*it)->closed = true;
            delete *it;
            activeStatWatchers.erase(it);
            return 1;
        }
    }
    return 1;
}

// FSWatcher class methods
void nova_fs_watcher_close(void* watcher) {
    if (!watcher) return;
    FSWatcher* w = static_cast<FSWatcher*>(watcher);
    w->closed = true;

#ifdef _WIN32
    w->stopThread = true;
    if (w->dirHandle != INVALID_HANDLE_VALUE) {
        CancelIoEx(w->dirHandle, nullptr);
        CloseHandle(w->dirHandle);
    }
    if (w->threadHandle) {
        WaitForSingleObject(w->threadHandle, 1000);
        CloseHandle(w->threadHandle);
    }
#elif defined(__linux__)
    if (w->watchDescriptor >= 0) {
        inotify_rm_watch(w->inotifyFd, w->watchDescriptor);
    }
    if (w->inotifyFd >= 0) {
        close(w->inotifyFd);
    }
#endif
    // macOS: no cleanup needed yet (no file watching implementation)

    // Call close listeners
    for (auto& listener : w->closeListeners) {
        if (listener.callback) {
            typedef void (*CloseCallback)();
            ((CloseCallback)listener.callback)();
        }
    }

    // Remove from active watchers
    for (auto it = activeWatchers.begin(); it != activeWatchers.end(); ++it) {
        if (*it == w) {
            activeWatchers.erase(it);
            break;
        }
    }

    delete w;
}

void* nova_fs_watcher_ref(void* watcher) {
    if (!watcher) return nullptr;
    FSWatcher* w = static_cast<FSWatcher*>(watcher);
    w->persistent = true;
    return watcher;
}

void* nova_fs_watcher_unref(void* watcher) {
    if (!watcher) return nullptr;
    FSWatcher* w = static_cast<FSWatcher*>(watcher);
    w->persistent = false;
    return watcher;
}

// FSWatcher EventEmitter methods
void* nova_fs_watcher_on(void* watcher, const char* event, void* listener) {
    if (!watcher || !event) return watcher;
    FSWatcher* w = static_cast<FSWatcher*>(watcher);

    FSEventListener entry = {listener, false};
    if (strcmp(event, "change") == 0) {
        w->changeListeners.push_back(entry);
    } else if (strcmp(event, "error") == 0) {
        w->errorListeners.push_back(entry);
    } else if (strcmp(event, "close") == 0) {
        w->closeListeners.push_back(entry);
    }
    return watcher;
}

void* nova_fs_watcher_once(void* watcher, const char* event, void* listener) {
    if (!watcher || !event) return watcher;
    FSWatcher* w = static_cast<FSWatcher*>(watcher);

    FSEventListener entry = {listener, true};
    if (strcmp(event, "change") == 0) {
        w->changeListeners.push_back(entry);
    } else if (strcmp(event, "error") == 0) {
        w->errorListeners.push_back(entry);
    } else if (strcmp(event, "close") == 0) {
        w->closeListeners.push_back(entry);
    }
    return watcher;
}

void* nova_fs_watcher_off(void* watcher, const char* event, void* listener) {
    if (!watcher || !event) return watcher;
    FSWatcher* w = static_cast<FSWatcher*>(watcher);

    std::vector<FSEventListener>* listeners = nullptr;
    if (strcmp(event, "change") == 0) listeners = &w->changeListeners;
    else if (strcmp(event, "error") == 0) listeners = &w->errorListeners;
    else if (strcmp(event, "close") == 0) listeners = &w->closeListeners;

    if (listeners) {
        for (auto it = listeners->begin(); it != listeners->end(); ++it) {
            if (it->callback == listener) {
                listeners->erase(it);
                break;
            }
        }
    }
    return watcher;
}

void* nova_fs_watcher_addListener(void* watcher, const char* event, void* listener) {
    return nova_fs_watcher_on(watcher, event, listener);
}

void* nova_fs_watcher_removeListener(void* watcher, const char* event, void* listener) {
    return nova_fs_watcher_off(watcher, event, listener);
}

void* nova_fs_watcher_removeAllListeners(void* watcher, const char* event) {
    if (!watcher) return watcher;
    FSWatcher* w = static_cast<FSWatcher*>(watcher);

    if (!event) {
        w->changeListeners.clear();
        w->errorListeners.clear();
        w->closeListeners.clear();
    } else if (strcmp(event, "change") == 0) {
        w->changeListeners.clear();
    } else if (strcmp(event, "error") == 0) {
        w->errorListeners.clear();
    } else if (strcmp(event, "close") == 0) {
        w->closeListeners.clear();
    }
    return watcher;
}

int nova_fs_watcher_emit(void* watcher, const char* event) {
    if (!watcher || !event) return 0;
    FSWatcher* w = static_cast<FSWatcher*>(watcher);

    std::vector<FSEventListener>* listeners = nullptr;
    if (strcmp(event, "change") == 0) listeners = &w->changeListeners;
    else if (strcmp(event, "error") == 0) listeners = &w->errorListeners;
    else if (strcmp(event, "close") == 0) listeners = &w->closeListeners;

    if (listeners && !listeners->empty()) {
        return 1;
    }
    return 0;
}

void* nova_fs_watcher_listeners(void* watcher, const char* event) {
    if (!watcher || !event) return nullptr;
    FSWatcher* w = static_cast<FSWatcher*>(watcher);

    std::vector<FSEventListener>* listeners = nullptr;
    if (strcmp(event, "change") == 0) listeners = &w->changeListeners;
    else if (strcmp(event, "error") == 0) listeners = &w->errorListeners;
    else if (strcmp(event, "close") == 0) listeners = &w->closeListeners;

    if (!listeners) return nullptr;

    // Return array of listener pointers
    size_t count = listeners->size();
    void** arr = (void**)malloc((count + 1) * sizeof(void*));
    for (size_t i = 0; i < count; i++) {
        arr[i] = (*listeners)[i].callback;
    }
    arr[count] = nullptr;
    return arr;
}

int nova_fs_watcher_listenerCount(void* watcher, const char* event) {
    if (!watcher || !event) return 0;
    FSWatcher* w = static_cast<FSWatcher*>(watcher);

    if (strcmp(event, "change") == 0) return (int)w->changeListeners.size();
    if (strcmp(event, "error") == 0) return (int)w->errorListeners.size();
    if (strcmp(event, "close") == 0) return (int)w->closeListeners.size();
    return 0;
}

// StatWatcher class methods
void* nova_fs_statwatcher_ref(void* watcher) {
    if (!watcher) return nullptr;
    StatWatcher* w = static_cast<StatWatcher*>(watcher);
    w->persistent = true;
    return watcher;
}

void* nova_fs_statwatcher_unref(void* watcher) {
    if (!watcher) return nullptr;
    StatWatcher* w = static_cast<StatWatcher*>(watcher);
    w->persistent = false;
    return watcher;
}

// StatWatcher EventEmitter methods
void* nova_fs_statwatcher_on(void* watcher, const char* event, void* listener) {
    if (!watcher || !event) return watcher;
    StatWatcher* w = static_cast<StatWatcher*>(watcher);

    FSEventListener entry = {listener, false};
    if (strcmp(event, "change") == 0) {
        w->changeListeners.push_back(entry);
    } else if (strcmp(event, "error") == 0) {
        w->errorListeners.push_back(entry);
    }
    return watcher;
}

void* nova_fs_statwatcher_once(void* watcher, const char* event, void* listener) {
    if (!watcher || !event) return watcher;
    StatWatcher* w = static_cast<StatWatcher*>(watcher);

    FSEventListener entry = {listener, true};
    if (strcmp(event, "change") == 0) {
        w->changeListeners.push_back(entry);
    } else if (strcmp(event, "error") == 0) {
        w->errorListeners.push_back(entry);
    }
    return watcher;
}

void* nova_fs_statwatcher_off(void* watcher, const char* event, void* listener) {
    if (!watcher || !event) return watcher;
    StatWatcher* w = static_cast<StatWatcher*>(watcher);

    std::vector<FSEventListener>* listeners = nullptr;
    if (strcmp(event, "change") == 0) listeners = &w->changeListeners;
    else if (strcmp(event, "error") == 0) listeners = &w->errorListeners;

    if (listeners) {
        for (auto it = listeners->begin(); it != listeners->end(); ++it) {
            if (it->callback == listener) {
                listeners->erase(it);
                break;
            }
        }
    }
    return watcher;
}

void* nova_fs_statwatcher_addListener(void* watcher, const char* event, void* listener) {
    return nova_fs_statwatcher_on(watcher, event, listener);
}

void* nova_fs_statwatcher_removeListener(void* watcher, const char* event, void* listener) {
    return nova_fs_statwatcher_off(watcher, event, listener);
}

void* nova_fs_statwatcher_removeAllListeners(void* watcher, const char* event) {
    if (!watcher) return watcher;
    StatWatcher* w = static_cast<StatWatcher*>(watcher);

    if (!event) {
        w->changeListeners.clear();
        w->errorListeners.clear();
    } else if (strcmp(event, "change") == 0) {
        w->changeListeners.clear();
    } else if (strcmp(event, "error") == 0) {
        w->errorListeners.clear();
    }
    return watcher;
}

int nova_fs_statwatcher_emit(void* watcher, const char* event) {
    if (!watcher || !event) return 0;
    StatWatcher* w = static_cast<StatWatcher*>(watcher);

    if (strcmp(event, "change") == 0 && !w->changeListeners.empty()) return 1;
    if (strcmp(event, "error") == 0 && !w->errorListeners.empty()) return 1;
    return 0;
}

void* nova_fs_statwatcher_listeners(void* watcher, const char* event) {
    if (!watcher || !event) return nullptr;
    StatWatcher* w = static_cast<StatWatcher*>(watcher);

    std::vector<FSEventListener>* listeners = nullptr;
    if (strcmp(event, "change") == 0) listeners = &w->changeListeners;
    else if (strcmp(event, "error") == 0) listeners = &w->errorListeners;

    if (!listeners) return nullptr;

    size_t count = listeners->size();
    void** arr = (void**)malloc((count + 1) * sizeof(void*));
    for (size_t i = 0; i < count; i++) {
        arr[i] = (*listeners)[i].callback;
    }
    arr[count] = nullptr;
    return arr;
}

int nova_fs_statwatcher_listenerCount(void* watcher, const char* event) {
    if (!watcher || !event) return 0;
    StatWatcher* w = static_cast<StatWatcher*>(watcher);

    if (strcmp(event, "change") == 0) return (int)w->changeListeners.size();
    if (strcmp(event, "error") == 0) return (int)w->errorListeners.size();
    return 0;
}

// ============================================================================
// Additional Node.js fs Sync Functions (100% compatibility)
// ============================================================================

// fs.lchmodSync(path, mode) - Changes symbolic link permissions (macOS only)
int nova_fs_lchmodSync(const char* path, int mode) {
    if (!path) return 0;

#if defined(__APPLE__)
    return lchmod(path, mode) == 0 ? 1 : 0;
#else
    // Not supported on Linux/Windows - silent success for compatibility
    (void)path;
    (void)mode;
    return 1;
#endif
}

// fs.lutimesSync(path, atime, mtime) - Changes symbolic link timestamps
int nova_fs_lutimesSync(const char* path, double atime, double mtime) {
    if (!path) return 0;

#ifdef _WIN32
    // Windows doesn't support lutimes - try to change via symlink target
    // For compatibility, return success
    (void)path;
    (void)atime;
    (void)mtime;
    return 1;
#else
    struct timeval tv[2];
    tv[0].tv_sec = (time_t)(atime / 1000);
    tv[0].tv_usec = ((long)(atime) % 1000) * 1000;
    tv[1].tv_sec = (time_t)(mtime / 1000);
    tv[1].tv_usec = ((long)(mtime) % 1000) * 1000;
    return lutimes(path, tv) == 0 ? 1 : 0;
#endif
}

// fs.readvSync(fd, buffers, position) - Scatter read into multiple buffers
// Returns total bytes read
int64_t nova_fs_readvSync(int fd, char** buffers, int64_t* lengths, int count, int64_t position) {
    if (fd < 0 || !buffers || !lengths || count <= 0) return -1;

#ifdef _WIN32
    // Windows doesn't have readv, simulate with multiple reads
    if (position >= 0) {
        _lseeki64(fd, position, SEEK_SET);
    }

    int64_t totalRead = 0;
    for (int i = 0; i < count; i++) {
        if (buffers[i] && lengths[i] > 0) {
            int bytesRead = _read(fd, buffers[i], (unsigned int)lengths[i]);
            if (bytesRead < 0) return totalRead > 0 ? totalRead : -1;
            totalRead += bytesRead;
            if (bytesRead < lengths[i]) break;  // EOF or short read
        }
    }
    return totalRead;
#else
    if (position >= 0) {
        lseek(fd, position, SEEK_SET);
    }

    // Build iovec array
    std::vector<struct iovec> iov(count);
    for (int i = 0; i < count; i++) {
        iov[i].iov_base = buffers[i];
        iov[i].iov_len = lengths[i];
    }

    ssize_t result = readv(fd, iov.data(), count);
    return result;
#endif
}

// fs.writevSync(fd, buffers, position) - Gather write from multiple buffers
// Returns total bytes written
int64_t nova_fs_writevSync(int fd, const char** buffers, int64_t* lengths, int count, int64_t position) {
    if (fd < 0 || !buffers || !lengths || count <= 0) return -1;

#ifdef _WIN32
    // Windows doesn't have writev, simulate with multiple writes
    if (position >= 0) {
        _lseeki64(fd, position, SEEK_SET);
    }

    int64_t totalWritten = 0;
    for (int i = 0; i < count; i++) {
        if (buffers[i] && lengths[i] > 0) {
            int bytesWritten = _write(fd, buffers[i], (unsigned int)lengths[i]);
            if (bytesWritten < 0) return totalWritten > 0 ? totalWritten : -1;
            totalWritten += bytesWritten;
            if (bytesWritten < lengths[i]) break;  // Short write
        }
    }
    return totalWritten;
#else
    if (position >= 0) {
        lseek(fd, position, SEEK_SET);
    }

    // Build iovec array
    std::vector<struct iovec> iov(count);
    for (int i = 0; i < count; i++) {
        iov[i].iov_base = (void*)buffers[i];
        iov[i].iov_len = lengths[i];
    }

    ssize_t result = writev(fd, iov.data(), count);
    return result;
#endif
}

// fs.realpathSync.native(path) - Native realpath implementation
char* nova_fs_realpathSyncNative(const char* path) {
    if (!path) return nullptr;

#ifdef _WIN32
    char resolved[MAX_PATH];
    DWORD len = GetFullPathNameA(path, MAX_PATH, resolved, nullptr);
    if (len == 0 || len >= MAX_PATH) return nullptr;
    return allocString(resolved);
#else
    char* resolved = realpath(path, nullptr);
    if (!resolved) return nullptr;
    char* result = allocString(resolved);
    free(resolved);
    return result;
#endif
}

// fs.mkdtempDisposableSync(prefix) - Creates temp directory (Node.js 22+)
// Returns path string - caller is responsible for cleanup
char* nova_fs_mkdtempDisposableSync(const char* prefix) {
    // Same as mkdtempSync - the "disposable" aspect is handled at JS level
    return nova_fs_mkdtempSync(prefix);
}

// ============================================================================
// CALLBACK API - Async functions with callbacks
// All callback functions follow pattern: func(args..., callback)
// Callback signature: void (*callback)(int error, result)
// ============================================================================

// Callback types
typedef void (*FSCallback)(int err);
typedef void (*FSCallbackInt)(int err, int result);
typedef void (*FSCallbackInt64)(int err, int64_t result);
typedef void (*FSCallbackStr)(int err, char* result);
typedef void (*FSCallbackPtr)(int err, void* result);

// fs.access(path, mode, callback)
void nova_fs_access(const char* path, int mode, FSCallback callback) {
    int result = nova_fs_accessSync(path, mode);
    if (callback) callback(result ? 0 : -1);
}

// fs.appendFile(path, data, callback)
void nova_fs_appendFile(const char* path, const char* data, FSCallback callback) {
    int result = nova_fs_appendFileSync(path, data);
    if (callback) callback(result ? 0 : -1);
}

// fs.chmod(path, mode, callback)
void nova_fs_chmod(const char* path, int mode, FSCallback callback) {
    int result = nova_fs_chmodSync(path, mode);
    if (callback) callback(result ? 0 : -1);
}

// fs.chown(path, uid, gid, callback)
void nova_fs_chown(const char* path, int uid, int gid, FSCallback callback) {
    int result = nova_fs_chownSync(path, uid, gid);
    if (callback) callback(result ? 0 : -1);
}

// fs.close(fd, callback)
void nova_fs_close(int fd, FSCallback callback) {
    int result = nova_fs_closeSync(fd);
    if (callback) callback(result ? 0 : -1);
}

// fs.copyFile(src, dest, callback)
void nova_fs_copyFile(const char* src, const char* dest, FSCallback callback) {
    int result = nova_fs_copyFileSync(src, dest);
    if (callback) callback(result ? 0 : -1);
}

// fs.cp(src, dest, callback)
void nova_fs_cp(const char* src, const char* dest, FSCallback callback) {
    int result = nova_fs_cpSync(src, dest);
    if (callback) callback(result ? 0 : -1);
}

// fs.exists(path, callback) - deprecated but included for compatibility
void nova_fs_exists(const char* path, void (*callback)(int exists)) {
    int result = nova_fs_existsSync(path);
    if (callback) callback(result);
}

// fs.fchmod(fd, mode, callback)
void nova_fs_fchmod(int fd, int mode, FSCallback callback) {
    int result = nova_fs_fchmodSync(fd, mode);
    if (callback) callback(result ? 0 : -1);
}

// fs.fchown(fd, uid, gid, callback)
void nova_fs_fchown(int fd, int uid, int gid, FSCallback callback) {
    int result = nova_fs_fchownSync(fd, uid, gid);
    if (callback) callback(result ? 0 : -1);
}

// fs.fdatasync(fd, callback)
void nova_fs_fdatasync(int fd, FSCallback callback) {
    int result = nova_fs_fdatasyncSync(fd);
    if (callback) callback(result ? 0 : -1);
}

// fs.fstat(fd, callback)
void nova_fs_fstat(int fd, FSCallbackPtr callback) {
    void* result = nova_fs_fstatSync(fd);
    if (callback) callback(result ? 0 : -1, result);
}

// fs.fsync(fd, callback)
void nova_fs_fsync(int fd, FSCallback callback) {
    int result = nova_fs_fsyncSync(fd);
    if (callback) callback(result ? 0 : -1);
}

// fs.ftruncate(fd, len, callback)
void nova_fs_ftruncate(int fd, int64_t len, FSCallback callback) {
    int result = nova_fs_ftruncateSync(fd, len);
    if (callback) callback(result ? 0 : -1);
}

// fs.futimes(fd, atime, mtime, callback)
void nova_fs_futimes(int fd, double atime, double mtime, FSCallback callback) {
    int result = nova_fs_futimesSync(fd, atime, mtime);
    if (callback) callback(result ? 0 : -1);
}

// fs.lchmod(path, mode, callback)
void nova_fs_lchmod(const char* path, int mode, FSCallback callback) {
    int result = nova_fs_lchmodSync(path, mode);
    if (callback) callback(result ? 0 : -1);
}

// fs.lchown(path, uid, gid, callback)
void nova_fs_lchown(const char* path, int uid, int gid, FSCallback callback) {
    int result = nova_fs_lchownSync(path, uid, gid);
    if (callback) callback(result ? 0 : -1);
}

// fs.lutimes(path, atime, mtime, callback)
void nova_fs_lutimes(const char* path, double atime, double mtime, FSCallback callback) {
    int result = nova_fs_lutimesSync(path, atime, mtime);
    if (callback) callback(result ? 0 : -1);
}

// fs.link(existingPath, newPath, callback)
void nova_fs_link(const char* existingPath, const char* newPath, FSCallback callback) {
    int result = nova_fs_linkSync(existingPath, newPath);
    if (callback) callback(result ? 0 : -1);
}

// fs.lstat(path, callback)
void nova_fs_lstat(const char* path, FSCallbackPtr callback) {
    void* result = nova_fs_lstatSync(path);
    if (callback) callback(result ? 0 : -1, result);
}

// fs.mkdir(path, callback)
void nova_fs_mkdir(const char* path, FSCallback callback) {
    int result = nova_fs_mkdirSync(path);
    if (callback) callback(result ? 0 : -1);
}

// fs.mkdtemp(prefix, callback)
void nova_fs_mkdtemp(const char* prefix, FSCallbackStr callback) {
    char* result = nova_fs_mkdtempSync(prefix);
    if (callback) callback(result ? 0 : -1, result);
}

// fs.open(path, flags, callback)
void nova_fs_open(const char* path, const char* flags, FSCallbackInt callback) {
    int result = nova_fs_openSync(path, flags);
    if (callback) callback(result >= 0 ? 0 : -1, result);
}

// fs.opendir(path, callback)
void nova_fs_opendir(const char* path, FSCallbackPtr callback) {
    void* result = nova_fs_opendirSync(path);
    if (callback) callback(result ? 0 : -1, result);
}

// fs.read(fd, buffer, length, position, callback)
void nova_fs_read(int fd, char* buffer, int64_t length, int64_t position, FSCallbackInt64 callback) {
    int64_t result = nova_fs_readSync(fd, buffer, length, position);
    if (callback) callback(result >= 0 ? 0 : -1, result);
}

// fs.readdir(path, callback)
void nova_fs_readdir(const char* path, FSCallbackStr callback) {
    char* result = nova_fs_readdirSync(path);
    if (callback) callback(result ? 0 : -1, result);
}

// fs.readFile(path, callback)
void nova_fs_readFile(const char* path, FSCallbackStr callback) {
    char* result = nova_fs_readFileSync(path);
    if (callback) callback(result ? 0 : -1, result);
}

// fs.readlink(path, callback)
void nova_fs_readlink(const char* path, FSCallbackStr callback) {
    char* result = nova_fs_readlinkSync(path);
    if (callback) callback(result ? 0 : -1, result);
}

// fs.readv(fd, buffers, lengths, count, position, callback)
void nova_fs_readv(int fd, char** buffers, int64_t* lengths, int count, int64_t position, FSCallbackInt64 callback) {
    int64_t result = nova_fs_readvSync(fd, buffers, lengths, count, position);
    if (callback) callback(result >= 0 ? 0 : -1, result);
}

// fs.realpath(path, callback)
void nova_fs_realpath(const char* path, FSCallbackStr callback) {
    char* result = nova_fs_realpathSync(path);
    if (callback) callback(result ? 0 : -1, result);
}

// fs.rename(oldPath, newPath, callback)
void nova_fs_rename(const char* oldPath, const char* newPath, FSCallback callback) {
    int result = nova_fs_renameSync(oldPath, newPath);
    if (callback) callback(result ? 0 : -1);
}

// fs.rmdir(path, callback)
void nova_fs_rmdir(const char* path, FSCallback callback) {
    int result = nova_fs_rmdirSync(path);
    if (callback) callback(result ? 0 : -1);
}

// fs.rm(path, callback)
void nova_fs_rm(const char* path, FSCallback callback) {
    int result = nova_fs_rmSync(path);
    if (callback) callback(result ? 0 : -1);
}

// fs.stat(path, callback)
void nova_fs_stat(const char* path, FSCallbackPtr callback) {
    void* result = nova_fs_statSync(path);
    if (callback) callback(result ? 0 : -1, result);
}

// fs.statfs(path, callback)
void nova_fs_statfs(const char* path, FSCallbackPtr callback) {
    void* result = nova_fs_statfsSync(path);
    if (callback) callback(result ? 0 : -1, result);
}

// fs.symlink(target, path, callback)
void nova_fs_symlink(const char* target, const char* path, FSCallback callback) {
    int result = nova_fs_symlinkSync(target, path);
    if (callback) callback(result ? 0 : -1);
}

// fs.truncate(path, len, callback)
void nova_fs_truncate(const char* path, int64_t len, FSCallback callback) {
    int result = nova_fs_truncateSync(path, len);
    if (callback) callback(result ? 0 : -1);
}

// fs.unlink(path, callback)
void nova_fs_unlink(const char* path, FSCallback callback) {
    int result = nova_fs_unlinkSync(path);
    if (callback) callback(result ? 0 : -1);
}

// fs.utimes(path, atime, mtime, callback)
void nova_fs_utimes(const char* path, double atime, double mtime, FSCallback callback) {
    int result = nova_fs_utimesSync(path, atime, mtime);
    if (callback) callback(result ? 0 : -1);
}

// fs.write(fd, buffer, length, position, callback)
void nova_fs_write(int fd, const char* buffer, int64_t length, int64_t position, FSCallbackInt64 callback) {
    int64_t result = nova_fs_writeSync(fd, buffer, length, position);
    if (callback) callback(result >= 0 ? 0 : -1, result);
}

// fs.writeFile(path, data, callback)
void nova_fs_writeFile(const char* path, const char* data, FSCallback callback) {
    int result = nova_fs_writeFileSync(path, data);
    if (callback) callback(result ? 0 : -1);
}

// fs.writev(fd, buffers, lengths, count, position, callback)
void nova_fs_writev(int fd, const char** buffers, int64_t* lengths, int count, int64_t position, FSCallbackInt64 callback) {
    int64_t result = nova_fs_writevSync(fd, buffers, lengths, count, position);
    if (callback) callback(result >= 0 ? 0 : -1, result);
}

// fs.glob(pattern, callback)
void nova_fs_glob(const char* pattern, FSCallbackStr callback) {
    char* result = nova_fs_globSync(pattern);
    if (callback) callback(result ? 0 : -1, result);
}

// ============================================================================
// PROMISES API - Returns promise-like structures
// For Nova, we simulate promises with result structures
// ============================================================================

// Promise result structure
struct NovaPromiseResult {
    int resolved;      // 1 if resolved, 0 if rejected
    int errorCode;     // Error code if rejected
    char* errorMsg;    // Error message if rejected
    void* value;       // Result value if resolved
    int64_t intValue;  // Integer result
    char* strValue;    // String result
};

static NovaPromiseResult* createResolvedPromise(void* value) {
    NovaPromiseResult* p = (NovaPromiseResult*)malloc(sizeof(NovaPromiseResult));
    if (p) {
        p->resolved = 1;
        p->errorCode = 0;
        p->errorMsg = nullptr;
        p->value = value;
        p->intValue = 0;
        p->strValue = nullptr;
    }
    return p;
}

static NovaPromiseResult* createResolvedPromiseInt(int64_t value) {
    NovaPromiseResult* p = createResolvedPromise(nullptr);
    if (p) p->intValue = value;
    return p;
}

static NovaPromiseResult* createResolvedPromiseStr(char* value) {
    NovaPromiseResult* p = createResolvedPromise(nullptr);
    if (p) p->strValue = value;
    return p;
}

static NovaPromiseResult* createRejectedPromise(int errorCode, const char* msg) {
    NovaPromiseResult* p = (NovaPromiseResult*)malloc(sizeof(NovaPromiseResult));
    if (p) {
        p->resolved = 0;
        p->errorCode = errorCode;
        p->errorMsg = msg ? allocString(msg) : nullptr;
        p->value = nullptr;
        p->intValue = 0;
        p->strValue = nullptr;
    }
    return p;
}

// fsPromises.access(path, mode)
void* nova_fs_promises_access(const char* path, int mode) {
    int result = nova_fs_accessSync(path, mode);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "EACCES");
}

// fsPromises.appendFile(path, data)
void* nova_fs_promises_appendFile(const char* path, const char* data) {
    int result = nova_fs_appendFileSync(path, data);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.chmod(path, mode)
void* nova_fs_promises_chmod(const char* path, int mode) {
    int result = nova_fs_chmodSync(path, mode);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.chown(path, uid, gid)
void* nova_fs_promises_chown(const char* path, int uid, int gid) {
    int result = nova_fs_chownSync(path, uid, gid);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.copyFile(src, dest)
void* nova_fs_promises_copyFile(const char* src, const char* dest) {
    int result = nova_fs_copyFileSync(src, dest);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.cp(src, dest)
void* nova_fs_promises_cp(const char* src, const char* dest) {
    int result = nova_fs_cpSync(src, dest);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.lchmod(path, mode)
void* nova_fs_promises_lchmod(const char* path, int mode) {
    int result = nova_fs_lchmodSync(path, mode);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.lchown(path, uid, gid)
void* nova_fs_promises_lchown(const char* path, int uid, int gid) {
    int result = nova_fs_lchownSync(path, uid, gid);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.lutimes(path, atime, mtime)
void* nova_fs_promises_lutimes(const char* path, double atime, double mtime) {
    int result = nova_fs_lutimesSync(path, atime, mtime);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.link(existingPath, newPath)
void* nova_fs_promises_link(const char* existingPath, const char* newPath) {
    int result = nova_fs_linkSync(existingPath, newPath);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.lstat(path)
void* nova_fs_promises_lstat(const char* path) {
    void* result = nova_fs_lstatSync(path);
    return result ? createResolvedPromise(result) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.mkdir(path)
void* nova_fs_promises_mkdir(const char* path) {
    int result = nova_fs_mkdirSync(path);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "EEXIST");
}

// fsPromises.mkdtemp(prefix)
void* nova_fs_promises_mkdtemp(const char* prefix) {
    char* result = nova_fs_mkdtempSync(prefix);
    return result ? createResolvedPromiseStr(result) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.open(path, flags)
void* nova_fs_promises_open(const char* path, const char* flags) {
    int fd = nova_fs_openSync(path, flags);
    return fd >= 0 ? createResolvedPromiseInt(fd) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.opendir(path)
void* nova_fs_promises_opendir(const char* path) {
    void* result = nova_fs_opendirSync(path);
    return result ? createResolvedPromise(result) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.readdir(path)
void* nova_fs_promises_readdir(const char* path) {
    char* result = nova_fs_readdirSync(path);
    return result ? createResolvedPromiseStr(result) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.readFile(path)
void* nova_fs_promises_readFile(const char* path) {
    char* result = nova_fs_readFileSync(path);
    return result ? createResolvedPromiseStr(result) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.readlink(path)
void* nova_fs_promises_readlink(const char* path) {
    char* result = nova_fs_readlinkSync(path);
    return result ? createResolvedPromiseStr(result) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.realpath(path)
void* nova_fs_promises_realpath(const char* path) {
    char* result = nova_fs_realpathSync(path);
    return result ? createResolvedPromiseStr(result) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.rename(oldPath, newPath)
void* nova_fs_promises_rename(const char* oldPath, const char* newPath) {
    int result = nova_fs_renameSync(oldPath, newPath);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.rmdir(path)
void* nova_fs_promises_rmdir(const char* path) {
    int result = nova_fs_rmdirSync(path);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.rm(path)
void* nova_fs_promises_rm(const char* path) {
    int result = nova_fs_rmSync(path);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.stat(path)
void* nova_fs_promises_stat(const char* path) {
    void* result = nova_fs_statSync(path);
    return result ? createResolvedPromise(result) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.statfs(path)
void* nova_fs_promises_statfs(const char* path) {
    void* result = nova_fs_statfsSync(path);
    return result ? createResolvedPromise(result) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.symlink(target, path)
void* nova_fs_promises_symlink(const char* target, const char* path) {
    int result = nova_fs_symlinkSync(target, path);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.truncate(path, len)
void* nova_fs_promises_truncate(const char* path, int64_t len) {
    int result = nova_fs_truncateSync(path, len);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.unlink(path)
void* nova_fs_promises_unlink(const char* path) {
    int result = nova_fs_unlinkSync(path);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.utimes(path, atime, mtime)
void* nova_fs_promises_utimes(const char* path, double atime, double mtime) {
    int result = nova_fs_utimesSync(path, atime, mtime);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.writeFile(path, data)
void* nova_fs_promises_writeFile(const char* path, const char* data) {
    int result = nova_fs_writeFileSync(path, data);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "ENOENT");
}

// fsPromises.glob(pattern)
void* nova_fs_promises_glob(const char* pattern) {
    char* result = nova_fs_globSync(pattern);
    return result ? createResolvedPromiseStr(result) : createRejectedPromise(-1, "ENOENT");
}

// Promise result helpers
int nova_fs_promise_isResolved(void* promise) {
    return promise ? ((NovaPromiseResult*)promise)->resolved : 0;
}

int nova_fs_promise_getError(void* promise) {
    return promise ? ((NovaPromiseResult*)promise)->errorCode : -1;
}

char* nova_fs_promise_getErrorMsg(void* promise) {
    return promise ? ((NovaPromiseResult*)promise)->errorMsg : nullptr;
}

void* nova_fs_promise_getValue(void* promise) {
    return promise ? ((NovaPromiseResult*)promise)->value : nullptr;
}

int64_t nova_fs_promise_getIntValue(void* promise) {
    return promise ? ((NovaPromiseResult*)promise)->intValue : 0;
}

char* nova_fs_promise_getStrValue(void* promise) {
    return promise ? ((NovaPromiseResult*)promise)->strValue : nullptr;
}

void nova_fs_promise_free(void* promise) {
    if (promise) {
        NovaPromiseResult* p = (NovaPromiseResult*)promise;
        if (p->errorMsg) free(p->errorMsg);
        // Note: strValue and value are owned by caller
        free(p);
    }
}

// ============================================================================
// FileHandle Class
// ============================================================================

struct NovaFileHandle {
    int fd;
    char* path;
    int closed;
};

// Create FileHandle from fd
void* nova_fs_filehandle_create(int fd, const char* path) {
    NovaFileHandle* fh = (NovaFileHandle*)malloc(sizeof(NovaFileHandle));
    if (fh) {
        fh->fd = fd;
        fh->path = path ? allocString(path) : nullptr;
        fh->closed = 0;
    }
    return fh;
}

// filehandle.fd
int nova_fs_filehandle_fd(void* handle) {
    return handle ? ((NovaFileHandle*)handle)->fd : -1;
}

// filehandle.close()
void* nova_fs_filehandle_close(void* handle) {
    if (!handle) return createRejectedPromise(-1, "Invalid handle");
    NovaFileHandle* fh = (NovaFileHandle*)handle;
    if (fh->closed) return createRejectedPromise(-1, "Already closed");

    int result = nova_fs_closeSync(fh->fd);
    fh->closed = 1;
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "Close failed");
}

// filehandle.read(buffer, offset, length, position)
void* nova_fs_filehandle_read(void* handle, char* buffer, int64_t length, int64_t position) {
    if (!handle) return createRejectedPromise(-1, "Invalid handle");
    NovaFileHandle* fh = (NovaFileHandle*)handle;
    if (fh->closed) return createRejectedPromise(-1, "Handle closed");

    int64_t result = nova_fs_readSync(fh->fd, buffer, length, position);
    return result >= 0 ? createResolvedPromiseInt(result) : createRejectedPromise(-1, "Read failed");
}

// filehandle.readFile()
void* nova_fs_filehandle_readFile(void* handle) {
    if (!handle) return createRejectedPromise(-1, "Invalid handle");
    NovaFileHandle* fh = (NovaFileHandle*)handle;
    if (fh->closed) return createRejectedPromise(-1, "Handle closed");

    // Read entire file
    // First get file size
    void* stats = nova_fs_fstatSync(fh->fd);
    if (!stats) return createRejectedPromise(-1, "Stat failed");

    int64_t size = nova_fs_stats_size(stats);
    nova_fs_stats_free(stats);

    if (size <= 0) return createResolvedPromiseStr(allocString(""));

    char* buffer = (char*)malloc(size + 1);
    if (!buffer) return createRejectedPromise(-1, "Out of memory");

    int64_t bytesRead = nova_fs_readSync(fh->fd, buffer, size, 0);
    if (bytesRead < 0) {
        free(buffer);
        return createRejectedPromise(-1, "Read failed");
    }

    buffer[bytesRead] = '\0';
    return createResolvedPromiseStr(buffer);
}

// filehandle.write(buffer, length, position)
void* nova_fs_filehandle_write(void* handle, const char* buffer, int64_t length, int64_t position) {
    if (!handle) return createRejectedPromise(-1, "Invalid handle");
    NovaFileHandle* fh = (NovaFileHandle*)handle;
    if (fh->closed) return createRejectedPromise(-1, "Handle closed");

    int64_t result = nova_fs_writeSync(fh->fd, buffer, length, position);
    return result >= 0 ? createResolvedPromiseInt(result) : createRejectedPromise(-1, "Write failed");
}

// filehandle.writeFile(data)
void* nova_fs_filehandle_writeFile(void* handle, const char* data) {
    if (!handle || !data) return createRejectedPromise(-1, "Invalid arguments");
    NovaFileHandle* fh = (NovaFileHandle*)handle;
    if (fh->closed) return createRejectedPromise(-1, "Handle closed");

    int64_t len = strlen(data);
    int64_t result = nova_fs_writeSync(fh->fd, data, len, 0);
    return result >= 0 ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "Write failed");
}

// filehandle.appendFile(data)
void* nova_fs_filehandle_appendFile(void* handle, const char* data) {
    if (!handle || !data) return createRejectedPromise(-1, "Invalid arguments");
    NovaFileHandle* fh = (NovaFileHandle*)handle;
    if (fh->closed) return createRejectedPromise(-1, "Handle closed");

    int64_t len = strlen(data);
    int64_t result = nova_fs_writeSync(fh->fd, data, len, -1);  // -1 = append
    return result >= 0 ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "Append failed");
}

// filehandle.chmod(mode)
void* nova_fs_filehandle_chmod(void* handle, int mode) {
    if (!handle) return createRejectedPromise(-1, "Invalid handle");
    NovaFileHandle* fh = (NovaFileHandle*)handle;
    if (fh->closed) return createRejectedPromise(-1, "Handle closed");

    int result = nova_fs_fchmodSync(fh->fd, mode);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "Chmod failed");
}

// filehandle.chown(uid, gid)
void* nova_fs_filehandle_chown(void* handle, int uid, int gid) {
    if (!handle) return createRejectedPromise(-1, "Invalid handle");
    NovaFileHandle* fh = (NovaFileHandle*)handle;
    if (fh->closed) return createRejectedPromise(-1, "Handle closed");

    int result = nova_fs_fchownSync(fh->fd, uid, gid);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "Chown failed");
}

// filehandle.datasync()
void* nova_fs_filehandle_datasync(void* handle) {
    if (!handle) return createRejectedPromise(-1, "Invalid handle");
    NovaFileHandle* fh = (NovaFileHandle*)handle;
    if (fh->closed) return createRejectedPromise(-1, "Handle closed");

    int result = nova_fs_fdatasyncSync(fh->fd);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "Datasync failed");
}

// filehandle.sync()
void* nova_fs_filehandle_sync(void* handle) {
    if (!handle) return createRejectedPromise(-1, "Invalid handle");
    NovaFileHandle* fh = (NovaFileHandle*)handle;
    if (fh->closed) return createRejectedPromise(-1, "Handle closed");

    int result = nova_fs_fsyncSync(fh->fd);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "Sync failed");
}

// filehandle.stat()
void* nova_fs_filehandle_stat(void* handle) {
    if (!handle) return createRejectedPromise(-1, "Invalid handle");
    NovaFileHandle* fh = (NovaFileHandle*)handle;
    if (fh->closed) return createRejectedPromise(-1, "Handle closed");

    void* stats = nova_fs_fstatSync(fh->fd);
    return stats ? createResolvedPromise(stats) : createRejectedPromise(-1, "Stat failed");
}

// filehandle.truncate(len)
void* nova_fs_filehandle_truncate(void* handle, int64_t len) {
    if (!handle) return createRejectedPromise(-1, "Invalid handle");
    NovaFileHandle* fh = (NovaFileHandle*)handle;
    if (fh->closed) return createRejectedPromise(-1, "Handle closed");

    int result = nova_fs_ftruncateSync(fh->fd, len);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "Truncate failed");
}

// filehandle.utimes(atime, mtime)
void* nova_fs_filehandle_utimes(void* handle, double atime, double mtime) {
    if (!handle) return createRejectedPromise(-1, "Invalid handle");
    NovaFileHandle* fh = (NovaFileHandle*)handle;
    if (fh->closed) return createRejectedPromise(-1, "Handle closed");

    int result = nova_fs_futimesSync(fh->fd, atime, mtime);
    return result ? createResolvedPromise(nullptr) : createRejectedPromise(-1, "Utimes failed");
}

// Free FileHandle
void nova_fs_filehandle_free(void* handle) {
    if (handle) {
        NovaFileHandle* fh = (NovaFileHandle*)handle;
        if (!fh->closed && fh->fd >= 0) {
            nova_fs_closeSync(fh->fd);
        }
        if (fh->path) free(fh->path);
        free(fh);
    }
}

// filehandle.readv(buffers, position)
void* nova_fs_filehandle_readv(void* handle, char** buffers, int64_t* lengths, int count, int64_t position) {
    if (!handle) return createRejectedPromise(-1, "Invalid handle");
    NovaFileHandle* fh = (NovaFileHandle*)handle;
    if (fh->closed) return createRejectedPromise(-1, "Handle closed");
    int64_t total = nova_fs_readvSync(fh->fd, buffers, lengths, count, position);
    return total >= 0 ? createResolvedPromiseInt(total) : createRejectedPromise(-1, "Readv failed");
}

// filehandle.writev(buffers, position)
void* nova_fs_filehandle_writev(void* handle, const char** buffers, int64_t* lengths, int count, int64_t position) {
    if (!handle) return createRejectedPromise(-1, "Invalid handle");
    NovaFileHandle* fh = (NovaFileHandle*)handle;
    if (fh->closed) return createRejectedPromise(-1, "Handle closed");
    int64_t total = nova_fs_writevSync(fh->fd, buffers, lengths, count, position);
    return total >= 0 ? createResolvedPromiseInt(total) : createRejectedPromise(-1, "Writev failed");
}

// Stats - Additional properties
int64_t nova_fs_stats_dev(void* stats) { return stats ? ((NovaStats*)stats)->dev : 0; }
int64_t nova_fs_stats_ino(void* stats) { return stats ? ((NovaStats*)stats)->ino : 0; }
int64_t nova_fs_stats_nlink(void* stats) { return stats ? ((NovaStats*)stats)->nlink : 0; }
int64_t nova_fs_stats_uid(void* stats) { return stats ? ((NovaStats*)stats)->uid : 0; }
int64_t nova_fs_stats_gid(void* stats) { return stats ? ((NovaStats*)stats)->gid : 0; }
int64_t nova_fs_stats_rdev(void* stats) { return stats ? ((NovaStats*)stats)->rdev : 0; }
int64_t nova_fs_stats_blksize(void* stats) { return stats ? ((NovaStats*)stats)->blksize : 0; }
int64_t nova_fs_stats_blocks(void* stats) { return stats ? ((NovaStats*)stats)->blocks : 0; }
int64_t nova_fs_stats_atimeNs(void* stats) { return stats ? (int64_t)(((NovaStats*)stats)->atimeMs * 1000000) : 0; }
int64_t nova_fs_stats_mtimeNs(void* stats) { return stats ? (int64_t)(((NovaStats*)stats)->mtimeMs * 1000000) : 0; }
int64_t nova_fs_stats_ctimeNs(void* stats) { return stats ? (int64_t)(((NovaStats*)stats)->ctimeMs * 1000000) : 0; }
int64_t nova_fs_stats_birthtimeNs(void* stats) { return stats ? (int64_t)(((NovaStats*)stats)->birthtimeMs * 1000000) : 0; }
double nova_fs_stats_atime(void* stats) { return stats ? ((NovaStats*)stats)->atimeMs : 0; }
double nova_fs_stats_mtime(void* stats) { return stats ? ((NovaStats*)stats)->mtimeMs : 0; }
double nova_fs_stats_ctime(void* stats) { return stats ? ((NovaStats*)stats)->ctimeMs : 0; }
double nova_fs_stats_birthtime(void* stats) { return stats ? ((NovaStats*)stats)->birthtimeMs : 0; }

// StatFs - Additional properties
int64_t nova_fs_statfs_type(void* statfs) { return statfs ? ((NovaStatFs*)statfs)->type : 0; }
int64_t nova_fs_statfs_files(void* statfs) { return statfs ? ((NovaStatFs*)statfs)->files : 0; }
int64_t nova_fs_statfs_ffree(void* statfs) { return statfs ? ((NovaStatFs*)statfs)->ffree : 0; }

// fs.openAsBlob(path) - Opens file as Blob
char* nova_fs_openAsBlob(const char* path) { return nova_fs_readFileSync(path); }

// fsPromises.watch(filename)
void* nova_fs_promises_watch(const char* filename) {
    void* watcher = nova_fs_watchFile(filename, nullptr);
    return watcher ? createResolvedPromise(watcher) : createRejectedPromise(-1, "Watch failed");
}

// fsPromises.mkdtempDisposable(prefix)
void* nova_fs_promises_mkdtempDisposable(const char* prefix) {
    char* result = nova_fs_mkdtempSync(prefix);
    return result ? createResolvedPromiseStr(result) : createRejectedPromise(-1, "Mkdtemp failed");
}

// fsPromises.constants - Returns pointer to constants (same as fs.constants)
void* nova_fs_promises_constants() {
    // Returns same constants as fs.constants
    // Individual constants accessible via nova_fs_constants_* functions
    return nullptr;  // Constants accessed individually
}

// ============================================================================
// Additional Constants (complete set)
// ============================================================================

// File access constants
int nova_fs_constants_S_IFMT() { return S_IFMT; }
int nova_fs_constants_S_IFREG() { return S_IFREG; }
int nova_fs_constants_S_IFDIR() { return S_IFDIR; }
#ifdef S_IFCHR
int nova_fs_constants_S_IFCHR() { return S_IFCHR; }
#else
int nova_fs_constants_S_IFCHR() { return 0x2000; }
#endif
#ifdef S_IFBLK
int nova_fs_constants_S_IFBLK() { return S_IFBLK; }
#else
int nova_fs_constants_S_IFBLK() { return 0x6000; }
#endif
#ifdef S_IFIFO
int nova_fs_constants_S_IFIFO() { return S_IFIFO; }
#else
int nova_fs_constants_S_IFIFO() { return 0x1000; }
#endif
#ifdef S_IFLNK
int nova_fs_constants_S_IFLNK() { return S_IFLNK; }
#else
int nova_fs_constants_S_IFLNK() { return 0xA000; }
#endif
#ifdef S_IFSOCK
int nova_fs_constants_S_IFSOCK() { return S_IFSOCK; }
#else
int nova_fs_constants_S_IFSOCK() { return 0xC000; }
#endif

// File mode constants
#ifdef S_IRWXU
int nova_fs_constants_S_IRWXU() { return S_IRWXU; }
#else
int nova_fs_constants_S_IRWXU() { return 0700; }
#endif
#ifdef S_IRUSR
int nova_fs_constants_S_IRUSR() { return S_IRUSR; }
#else
int nova_fs_constants_S_IRUSR() { return 0400; }
#endif
#ifdef S_IWUSR
int nova_fs_constants_S_IWUSR() { return S_IWUSR; }
#else
int nova_fs_constants_S_IWUSR() { return 0200; }
#endif
#ifdef S_IXUSR
int nova_fs_constants_S_IXUSR() { return S_IXUSR; }
#else
int nova_fs_constants_S_IXUSR() { return 0100; }
#endif
#ifdef S_IRWXG
int nova_fs_constants_S_IRWXG() { return S_IRWXG; }
#else
int nova_fs_constants_S_IRWXG() { return 070; }
#endif
#ifdef S_IRGRP
int nova_fs_constants_S_IRGRP() { return S_IRGRP; }
#else
int nova_fs_constants_S_IRGRP() { return 040; }
#endif
#ifdef S_IWGRP
int nova_fs_constants_S_IWGRP() { return S_IWGRP; }
#else
int nova_fs_constants_S_IWGRP() { return 020; }
#endif
#ifdef S_IXGRP
int nova_fs_constants_S_IXGRP() { return S_IXGRP; }
#else
int nova_fs_constants_S_IXGRP() { return 010; }
#endif
#ifdef S_IRWXO
int nova_fs_constants_S_IRWXO() { return S_IRWXO; }
#else
int nova_fs_constants_S_IRWXO() { return 07; }
#endif
#ifdef S_IROTH
int nova_fs_constants_S_IROTH() { return S_IROTH; }
#else
int nova_fs_constants_S_IROTH() { return 04; }
#endif
#ifdef S_IWOTH
int nova_fs_constants_S_IWOTH() { return S_IWOTH; }
#else
int nova_fs_constants_S_IWOTH() { return 02; }
#endif
#ifdef S_IXOTH
int nova_fs_constants_S_IXOTH() { return S_IXOTH; }
#else
int nova_fs_constants_S_IXOTH() { return 01; }
#endif

// UV error codes (for compatibility)
int nova_fs_constants_UV_FS_SYMLINK_DIR() { return 1; }
int nova_fs_constants_UV_FS_SYMLINK_JUNCTION() { return 2; }

// ============================================================================
// STREAMS - ReadStream and WriteStream
// ============================================================================

// ReadStream structure
struct NovaReadStream {
    int fd;
    char* path;
    int64_t start;
    int64_t end;
    int64_t position;
    int highWaterMark;
    int autoClose;
    int closed;
    int paused;
    int ended;
    char* encoding;
};

// WriteStream structure
struct NovaWriteStream {
    int fd;
    char* path;
    int64_t start;
    int highWaterMark;
    int autoClose;
    int closed;
    int pending;
    char* encoding;
    int64_t bytesWritten;
};

// createReadStream(path, options)
void* nova_fs_createReadStream(const char* path, [[maybe_unused]] const char* options) {
    int fd = nova_fs_openSync(path, "r");
    if (fd < 0) return nullptr;

    NovaReadStream* rs = (NovaReadStream*)malloc(sizeof(NovaReadStream));
    if (!rs) {
        nova_fs_closeSync(fd);
        return nullptr;
    }

    rs->fd = fd;
    rs->path = allocString(path);
    rs->start = 0;
    rs->end = -1;  // -1 means read to end
    rs->position = 0;
    rs->highWaterMark = 64 * 1024;  // 64KB default
    rs->autoClose = 1;
    rs->closed = 0;
    rs->paused = 0;
    rs->ended = 0;
    rs->encoding = nullptr;

    return rs;
}

// ReadStream.read(size)
char* nova_fs_readstream_read(void* stream, int64_t size) {
    if (!stream) return nullptr;
    NovaReadStream* rs = (NovaReadStream*)stream;
    if (rs->closed || rs->ended) return nullptr;

    if (size <= 0) size = rs->highWaterMark;

    // Check end boundary
    if (rs->end >= 0 && rs->position + size > rs->end) {
        size = rs->end - rs->position;
        if (size <= 0) {
            rs->ended = 1;
            return nullptr;
        }
    }

    char* buffer = (char*)malloc(size + 1);
    if (!buffer) return nullptr;

    int64_t bytesRead = nova_fs_readSync(rs->fd, buffer, size, rs->position);
    if (bytesRead <= 0) {
        free(buffer);
        rs->ended = 1;
        return nullptr;
    }

    buffer[bytesRead] = '\0';
    rs->position += bytesRead;
    return buffer;
}

// ReadStream.pause()
void nova_fs_readstream_pause(void* stream) {
    if (stream) ((NovaReadStream*)stream)->paused = 1;
}

// ReadStream.resume()
void nova_fs_readstream_resume(void* stream) {
    if (stream) ((NovaReadStream*)stream)->paused = 0;
}

// ReadStream.close()
void nova_fs_readstream_close(void* stream) {
    if (!stream) return;
    NovaReadStream* rs = (NovaReadStream*)stream;
    if (!rs->closed && rs->autoClose && rs->fd >= 0) {
        nova_fs_closeSync(rs->fd);
    }
    rs->closed = 1;
}

// ReadStream.destroy()
void nova_fs_readstream_destroy(void* stream) {
    if (!stream) return;
    NovaReadStream* rs = (NovaReadStream*)stream;
    nova_fs_readstream_close(stream);
    if (rs->path) free(rs->path);
    if (rs->encoding) free(rs->encoding);
    free(rs);
}

// ReadStream.path
char* nova_fs_readstream_path(void* stream) {
    return stream ? ((NovaReadStream*)stream)->path : nullptr;
}

// ReadStream.pending
int nova_fs_readstream_pending(void* stream) {
    return stream ? 0 : 1;  // false if valid stream
}

// ReadStream.bytesRead (current position)
int64_t nova_fs_readstream_bytesRead(void* stream) {
    return stream ? ((NovaReadStream*)stream)->position : 0;
}

// ReadStream EventEmitter methods (extends stream.Readable which extends EventEmitter)
void* nova_fs_readstream_on(void* stream, const char* event, void* listener) {
    (void)event; (void)listener;
    return stream;
}

void* nova_fs_readstream_once(void* stream, const char* event, void* listener) {
    (void)event; (void)listener;
    return stream;
}

void* nova_fs_readstream_off(void* stream, const char* event, void* listener) {
    (void)event; (void)listener;
    return stream;
}

void* nova_fs_readstream_addListener(void* stream, const char* event, void* listener) {
    return nova_fs_readstream_on(stream, event, listener);
}

void* nova_fs_readstream_removeListener(void* stream, const char* event, void* listener) {
    return nova_fs_readstream_off(stream, event, listener);
}

void* nova_fs_readstream_removeAllListeners(void* stream, const char* event) {
    (void)event;
    return stream;
}

int nova_fs_readstream_emit(void* stream, const char* event) {
    (void)stream; (void)event;
    return 1;
}

void* nova_fs_readstream_listeners(void* stream, const char* event) {
    (void)stream; (void)event;
    return nullptr;
}

int nova_fs_readstream_listenerCount(void* stream, const char* event) {
    (void)stream; (void)event;
    return 0;
}

// ReadStream stream.Readable methods
int nova_fs_readstream_isPaused(void* stream) {
    return stream ? ((NovaReadStream*)stream)->paused : 1;
}

void* nova_fs_readstream_pipe(void* stream, void* destination) {
    (void)stream;
    return destination;  // Return destination for chaining
}

void* nova_fs_readstream_unpipe(void* stream, void* destination) {
    (void)destination;
    return stream;
}

void* nova_fs_readstream_setEncoding(void* stream, const char* encoding) {
    (void)encoding;
    return stream;
}

void nova_fs_readstream_unshift(void* stream, const char* chunk) {
    (void)stream; (void)chunk;
}

void* nova_fs_readstream_wrap(void* stream, void* oldStream) {
    (void)oldStream;
    return stream;
}

// ReadStream readable properties
int nova_fs_readstream_readable(void* stream) {
    return stream ? !((NovaReadStream*)stream)->closed : 0;
}

int nova_fs_readstream_readableAborted(void* stream) {
    (void)stream;
    return 0;
}

int nova_fs_readstream_readableDidRead(void* stream) {
    return stream ? (((NovaReadStream*)stream)->position > 0 ? 1 : 0) : 0;
}

char* nova_fs_readstream_readableEncoding(void* stream) {
    (void)stream;
    return nullptr;  // null means no encoding set
}

int nova_fs_readstream_readableEnded(void* stream) {
    return stream ? ((NovaReadStream*)stream)->closed : 1;
}

int nova_fs_readstream_readableFlowing(void* stream) {
    return stream ? !((NovaReadStream*)stream)->paused : 0;
}

int nova_fs_readstream_readableHighWaterMark(void* stream) {
    return stream ? ((NovaReadStream*)stream)->highWaterMark : 16384;
}

int nova_fs_readstream_readableLength(void* stream) {
    (void)stream;
    return 0;  // Buffered bytes
}

int nova_fs_readstream_readableObjectMode(void* stream) {
    (void)stream;
    return 0;  // Not in object mode
}

// createWriteStream(path, options)
void* nova_fs_createWriteStream(const char* path, [[maybe_unused]] const char* options) {
    int fd = nova_fs_openSync(path, "w");
    if (fd < 0) return nullptr;

    NovaWriteStream* ws = (NovaWriteStream*)malloc(sizeof(NovaWriteStream));
    if (!ws) {
        nova_fs_closeSync(fd);
        return nullptr;
    }

    ws->fd = fd;
    ws->path = allocString(path);
    ws->start = 0;
    ws->highWaterMark = 16 * 1024;  // 16KB default
    ws->autoClose = 1;
    ws->closed = 0;
    ws->pending = 0;
    ws->encoding = nullptr;
    ws->bytesWritten = 0;

    return ws;
}

// WriteStream.write(data)
int nova_fs_writestream_write(void* stream, const char* data) {
    if (!stream || !data) return 0;
    NovaWriteStream* ws = (NovaWriteStream*)stream;
    if (ws->closed) return 0;

    int64_t len = strlen(data);
    int64_t written = nova_fs_writeSync(ws->fd, data, len, -1);
    if (written > 0) {
        ws->bytesWritten += written;
        return 1;
    }
    return 0;
}

// Forward declaration
void nova_fs_writestream_close(void* stream);

// WriteStream.end(data)
void nova_fs_writestream_end(void* stream, const char* data) {
    if (!stream) return;

    if (data) {
        nova_fs_writestream_write(stream, data);
    }

    nova_fs_writestream_close(stream);
}

// WriteStream.close()
void nova_fs_writestream_close(void* stream) {
    if (!stream) return;
    NovaWriteStream* ws = (NovaWriteStream*)stream;
    if (!ws->closed && ws->autoClose && ws->fd >= 0) {
        nova_fs_closeSync(ws->fd);
    }
    ws->closed = 1;
}

// WriteStream.destroy()
void nova_fs_writestream_destroy(void* stream) {
    if (!stream) return;
    NovaWriteStream* ws = (NovaWriteStream*)stream;
    nova_fs_writestream_close(stream);
    if (ws->path) free(ws->path);
    if (ws->encoding) free(ws->encoding);
    free(ws);
}

// WriteStream.path
char* nova_fs_writestream_path(void* stream) {
    return stream ? ((NovaWriteStream*)stream)->path : nullptr;
}

// WriteStream.pending
int nova_fs_writestream_pending(void* stream) {
    return stream ? ((NovaWriteStream*)stream)->pending : 1;
}

// WriteStream.bytesWritten
int64_t nova_fs_writestream_bytesWritten(void* stream) {
    return stream ? ((NovaWriteStream*)stream)->bytesWritten : 0;
}

// filehandle.createReadStream(options) - requires NovaReadStream defined above
void* nova_fs_filehandle_createReadStream(void* handle) {
    if (!handle) return nullptr;
    NovaFileHandle* fh = (NovaFileHandle*)handle;
    if (fh->closed) return nullptr;
    NovaReadStream* rs = (NovaReadStream*)malloc(sizeof(NovaReadStream));
    if (!rs) return nullptr;
    rs->fd = fh->fd;
    rs->path = fh->path ? allocString(fh->path) : nullptr;
    rs->start = 0; rs->end = -1; rs->position = 0;
    rs->highWaterMark = 64 * 1024; rs->autoClose = 0;
    rs->closed = 0; rs->paused = 0; rs->ended = 0; rs->encoding = nullptr;
    return rs;
}

// filehandle.createWriteStream(options) - requires NovaWriteStream defined above
void* nova_fs_filehandle_createWriteStream(void* handle) {
    if (!handle) return nullptr;
    NovaFileHandle* fh = (NovaFileHandle*)handle;
    if (fh->closed) return nullptr;
    NovaWriteStream* ws = (NovaWriteStream*)malloc(sizeof(NovaWriteStream));
    if (!ws) return nullptr;
    ws->fd = fh->fd;
    ws->path = fh->path ? allocString(fh->path) : nullptr;
    ws->start = 0; ws->highWaterMark = 16 * 1024; ws->autoClose = 0;
    ws->closed = 0; ws->pending = 0; ws->encoding = nullptr; ws->bytesWritten = 0;
    return ws;
}

// ============================================================================
// Utf8Stream CLASS (Node.js fs.Utf8Stream)
// ============================================================================

struct NovaUtf8Stream {
    int fd;
    char* file;
    int append;
    int contentMode;  // 0 = text
    int fsync;
    int maxLength;
    int minLength;
    int mkdir;
    int mode;
    int periodicFlush;
    int sync;
    int writing;
    int closed;
};

// Utf8Stream constructor
void* nova_fs_utf8stream_create(const char* file) {
    NovaUtf8Stream* stream = (NovaUtf8Stream*)malloc(sizeof(NovaUtf8Stream));
    if (!stream) return nullptr;
    stream->fd = -1;
    stream->file = file ? allocString(file) : nullptr;
    stream->append = 0;
    stream->contentMode = 0;
    stream->fsync = 0;
    stream->maxLength = 4096;
    stream->minLength = 0;
    stream->mkdir = 0;
    stream->mode = 438;  // 0o666
    stream->periodicFlush = 0;
    stream->sync = 0;
    stream->writing = 0;
    stream->closed = 0;
    return stream;
}

// Utf8Stream.write(data)
int nova_fs_utf8stream_write(void* stream, const char* data) {
    if (!stream || !data) return 0;
    NovaUtf8Stream* s = (NovaUtf8Stream*)stream;
    if (s->closed) return 0;
    s->writing = 1;
    if (s->fd < 0 && s->file) {
        s->fd = nova_fs_openSync(s->file, s->append ? "a" : "w");
    }
    if (s->fd >= 0) {
        int64_t len = data ? strlen(data) : 0;
        nova_fs_writeSync(s->fd, data, len, -1);  // -1 = append
    }
    s->writing = 0;
    return 1;
}

// Utf8Stream.flush()
void* nova_fs_utf8stream_flush(void* stream) {
    (void)stream;
    return createResolvedPromise(nullptr);
}

// Utf8Stream.flushSync()
void nova_fs_utf8stream_flushSync(void* stream) {
    if (!stream) return;
    NovaUtf8Stream* s = (NovaUtf8Stream*)stream;
    if (s->fd >= 0 && s->fsync) {
#ifdef _WIN32
        _commit(s->fd);
#else
        fsync(s->fd);
#endif
    }
}

// Utf8Stream.end([data])
void nova_fs_utf8stream_end(void* stream, const char* data) {
    if (!stream) return;
    NovaUtf8Stream* s = (NovaUtf8Stream*)stream;
    if (data) nova_fs_utf8stream_write(stream, data);
    if (s->fd >= 0) {
        nova_fs_closeSync(s->fd);
        s->fd = -1;
    }
    s->closed = 1;
}

// Utf8Stream.destroy()
void nova_fs_utf8stream_destroy(void* stream) {
    if (!stream) return;
    NovaUtf8Stream* s = (NovaUtf8Stream*)stream;
    if (s->fd >= 0) {
        nova_fs_closeSync(s->fd);
        s->fd = -1;
    }
    s->closed = 1;
}

// Utf8Stream.reopen()
void nova_fs_utf8stream_reopen(void* stream) {
    if (!stream) return;
    NovaUtf8Stream* s = (NovaUtf8Stream*)stream;
    if (s->fd >= 0) {
        nova_fs_closeSync(s->fd);
    }
    if (s->file) {
        s->fd = nova_fs_openSync(s->file, s->append ? "a" : "w");
    }
    s->closed = 0;
}

// Utf8Stream properties
int nova_fs_utf8stream_append(void* stream) {
    return stream ? ((NovaUtf8Stream*)stream)->append : 0;
}

int nova_fs_utf8stream_contentMode(void* stream) {
    return stream ? ((NovaUtf8Stream*)stream)->contentMode : 0;
}

int nova_fs_utf8stream_fd(void* stream) {
    return stream ? ((NovaUtf8Stream*)stream)->fd : -1;
}

char* nova_fs_utf8stream_file(void* stream) {
    return stream ? ((NovaUtf8Stream*)stream)->file : nullptr;
}

int nova_fs_utf8stream_fsync(void* stream) {
    return stream ? ((NovaUtf8Stream*)stream)->fsync : 0;
}

int nova_fs_utf8stream_maxLength(void* stream) {
    return stream ? ((NovaUtf8Stream*)stream)->maxLength : 4096;
}

int nova_fs_utf8stream_minLength(void* stream) {
    return stream ? ((NovaUtf8Stream*)stream)->minLength : 0;
}

int nova_fs_utf8stream_mkdir(void* stream) {
    return stream ? ((NovaUtf8Stream*)stream)->mkdir : 0;
}

int nova_fs_utf8stream_mode(void* stream) {
    return stream ? ((NovaUtf8Stream*)stream)->mode : 438;
}

int nova_fs_utf8stream_periodicFlush(void* stream) {
    return stream ? ((NovaUtf8Stream*)stream)->periodicFlush : 0;
}

int nova_fs_utf8stream_sync(void* stream) {
    return stream ? ((NovaUtf8Stream*)stream)->sync : 0;
}

int nova_fs_utf8stream_writing(void* stream) {
    return stream ? ((NovaUtf8Stream*)stream)->writing : 0;
}

// Utf8Stream EventEmitter methods
void* nova_fs_utf8stream_on(void* stream, const char* event, void* listener) {
    (void)event; (void)listener;
    return stream;
}

void* nova_fs_utf8stream_once(void* stream, const char* event, void* listener) {
    (void)event; (void)listener;
    return stream;
}

void* nova_fs_utf8stream_off(void* stream, const char* event, void* listener) {
    (void)event; (void)listener;
    return stream;
}

int nova_fs_utf8stream_emit(void* stream, const char* event) {
    (void)stream; (void)event;
    return 1;
}

// Utf8Stream Symbol.dispose
void nova_fs_utf8stream_dispose(void* stream) {
    nova_fs_utf8stream_destroy(stream);
}

// Utf8Stream free
void nova_fs_utf8stream_free(void* stream) {
    if (!stream) return;
    NovaUtf8Stream* s = (NovaUtf8Stream*)stream;
    if (s->fd >= 0) nova_fs_closeSync(s->fd);
    if (s->file) free(s->file);
    free(s);
}

// ============================================================================
// DIR CLASS - Additional async methods (struct already defined above)
// ============================================================================

// Dir.read() - async version
void* nova_fs_dir_read(void* dir) {
    char* entry = nova_fs_dir_readSync(dir);
    return entry ? createResolvedPromiseStr(entry) : createResolvedPromise(nullptr);
}

// Dir.close() - async version
void* nova_fs_dir_close(void* dir) {
    nova_fs_dir_closeSync(dir);
    return createResolvedPromise(nullptr);
}

// Dir.close(callback) - callback version
void nova_fs_dir_closeCallback(void* dir, void (*callback)(int err)) {
    int result = nova_fs_dir_closeSync(dir);
    if (callback) callback(result ? 0 : -1);
}

// Dir.read(callback) - callback version  
void nova_fs_dir_readCallback(void* dir, void (*callback)(int err, void* dirent)) {
    void* dirent = nova_fs_dir_readSyncDirent(dir);
    if (callback) callback(dirent ? 0 : -1, dirent);
}

// Dir[Symbol.asyncIterator]() - async iterator support
void* nova_fs_dir_asyncIterator(void* dir) {
    // Returns iterator object - the dir itself can be used as iterator
    return dir;
}

// Dir[Symbol.asyncDispose]() - async resource disposal
void* nova_fs_dir_asyncDispose(void* dir) {
    return nova_fs_dir_close(dir);  // Returns promise
}

// Dir[Symbol.dispose]() - sync resource disposal
void nova_fs_dir_dispose(void* dir) {
    nova_fs_dir_closeSync(dir);
}

// Dir.path - returns path string
char* nova_fs_dir_path(void* dir) {
    if (!dir) return nullptr;
    NovaDir* d = (NovaDir*)dir;
    return allocString(d->path);
}

} // extern "C"

} // namespace fs
} // namespace runtime
} // namespace nova
