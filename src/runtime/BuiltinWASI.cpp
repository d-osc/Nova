// Nova Builtin WASI Module Implementation
// Provides Node.js-compatible WebAssembly System Interface (WASI) API

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <chrono>
#include <random>
#include <mutex>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#endif

extern "C" {

// ============================================================================
// WASI Error Codes
// ============================================================================

#define WASI_ESUCCESS        0
#define WASI_E2BIG           1
#define WASI_EACCES          2
#define WASI_EADDRINUSE      3
#define WASI_EADDRNOTAVAIL   4
#define WASI_EAFNOSUPPORT    5
#define WASI_EAGAIN          6
#define WASI_EALREADY        7
#define WASI_EBADF           8
#define WASI_EBADMSG         9
#define WASI_EBUSY          10
#define WASI_ECANCELED      11
#define WASI_ECHILD         12
#define WASI_ECONNABORTED   13
#define WASI_ECONNREFUSED   14
#define WASI_ECONNRESET     15
#define WASI_EDEADLK        16
#define WASI_EDESTADDRREQ   17
#define WASI_EDOM           18
#define WASI_EDQUOT         19
#define WASI_EEXIST         20
#define WASI_EFAULT         21
#define WASI_EFBIG          22
#define WASI_EHOSTUNREACH   23
#define WASI_EIDRM          24
#define WASI_EILSEQ         25
#define WASI_EINPROGRESS    26
#define WASI_EINTR          27
#define WASI_EINVAL         28
#define WASI_EIO            29
#define WASI_EISCONN        30
#define WASI_EISDIR         31
#define WASI_ELOOP          32
#define WASI_EMFILE         33
#define WASI_EMLINK         34
#define WASI_EMSGSIZE       35
#define WASI_EMULTIHOP      36
#define WASI_ENAMETOOLONG   37
#define WASI_ENETDOWN       38
#define WASI_ENETRESET      39
#define WASI_ENETUNREACH    40
#define WASI_ENFILE         41
#define WASI_ENOBUFS        42
#define WASI_ENODEV         43
#define WASI_ENOENT         44
#define WASI_ENOEXEC        45
#define WASI_ENOLCK         46
#define WASI_ENOLINK        47
#define WASI_ENOMEM         48
#define WASI_ENOMSG         49
#define WASI_ENOPROTOOPT    50
#define WASI_ENOSPC         51
#define WASI_ENOSYS         52
#define WASI_ENOTCONN       53
#define WASI_ENOTDIR        54
#define WASI_ENOTEMPTY      55
#define WASI_ENOTRECOVERABLE 56
#define WASI_ENOTSOCK       57
#define WASI_ENOTSUP        58
#define WASI_ENOTTY         59
#define WASI_ENXIO          60
#define WASI_EOVERFLOW      61
#define WASI_EOWNERDEAD     62
#define WASI_EPERM          63
#define WASI_EPIPE          64
#define WASI_EPROTO         65
#define WASI_EPROTONOSUPPORT 66
#define WASI_EPROTOTYPE     67
#define WASI_ERANGE         68
#define WASI_EROFS          69
#define WASI_ESPIPE         70
#define WASI_ESRCH          71
#define WASI_ESTALE         72
#define WASI_ETIMEDOUT      73
#define WASI_ETXTBSY        74
#define WASI_EXDEV          75
#define WASI_ENOTCAPABLE    76

// ============================================================================
// WASI Clock IDs
// ============================================================================

#define WASI_CLOCK_REALTIME           0
#define WASI_CLOCK_MONOTONIC          1
#define WASI_CLOCK_PROCESS_CPUTIME_ID 2
#define WASI_CLOCK_THREAD_CPUTIME_ID  3

// ============================================================================
// WASI File Descriptor Flags
// ============================================================================

#define WASI_FDFLAG_APPEND   (1 << 0)
#define WASI_FDFLAG_DSYNC    (1 << 1)
#define WASI_FDFLAG_NONBLOCK (1 << 2)
#define WASI_FDFLAG_RSYNC    (1 << 3)
#define WASI_FDFLAG_SYNC     (1 << 4)

// ============================================================================
// WASI Instance Structure
// ============================================================================

struct NovaWASIPreopen {
    std::string guestPath;
    std::string hostPath;
    int fd;
};

// WASI iovec structure for read/write operations
struct WasiIovec {
    void* buf;
    size_t len;
};

struct WasiCiovec {
    const void* buf;
    size_t len;
};

struct NovaWASI {
    int64_t id;
    std::vector<std::string> args;
    std::unordered_map<std::string, std::string> env;
    std::vector<NovaWASIPreopen> preopens;
    int stdinFd;
    int stdoutFd;
    int stderrFd;
    bool returnOnExit;
    int exitCode;
    bool started;
    std::string version;
    std::unordered_map<int, std::string> fdPaths;
    int nextFd;
};

static std::atomic<int64_t> nextWasiId{1};

// ============================================================================
// WASI Constructor
// ============================================================================

void* nova_wasi_create() {
    NovaWASI* wasi = new NovaWASI();
    wasi->id = nextWasiId++;
    wasi->stdinFd = STDIN_FILENO;
    wasi->stdoutFd = STDOUT_FILENO;
    wasi->stderrFd = STDERR_FILENO;
    wasi->returnOnExit = true;
    wasi->exitCode = 0;
    wasi->started = false;
    wasi->version = "preview1";
    wasi->nextFd = 3;  // 0, 1, 2 are reserved

    // Default preopens
    wasi->fdPaths[0] = "<stdin>";
    wasi->fdPaths[1] = "<stdout>";
    wasi->fdPaths[2] = "<stderr>";

    return wasi;
}

void* nova_wasi_createWithOptions(const char** args, int argCount,
                                   const char** envKeys, const char** envValues, int envCount,
                                   const char** preopenGuest, const char** preopenHost, int preopenCount,
                                   int stdinFd, int stdoutFd, int stderrFd,
                                   int returnOnExit, const char* version) {
    NovaWASI* wasi = (NovaWASI*)nova_wasi_create();

    // Set args
    if (args && argCount > 0) {
        for (int i = 0; i < argCount; i++) {
            if (args[i]) wasi->args.push_back(args[i]);
        }
    }

    // Set env
    if (envKeys && envValues && envCount > 0) {
        for (int i = 0; i < envCount; i++) {
            if (envKeys[i]) {
                wasi->env[envKeys[i]] = envValues[i] ? envValues[i] : "";
            }
        }
    }

    // Set preopens
    if (preopenGuest && preopenHost && preopenCount > 0) {
        for (int i = 0; i < preopenCount; i++) {
            if (preopenGuest[i] && preopenHost[i]) {
                NovaWASIPreopen preopen;
                preopen.guestPath = preopenGuest[i];
                preopen.hostPath = preopenHost[i];
                preopen.fd = wasi->nextFd++;
                wasi->preopens.push_back(preopen);
                wasi->fdPaths[preopen.fd] = preopen.hostPath;
            }
        }
    }

    if (stdinFd >= 0) wasi->stdinFd = stdinFd;
    if (stdoutFd >= 0) wasi->stdoutFd = stdoutFd;
    if (stderrFd >= 0) wasi->stderrFd = stderrFd;
    wasi->returnOnExit = returnOnExit != 0;
    if (version) wasi->version = version;

    return wasi;
}

// ============================================================================
// WASI Configuration
// ============================================================================

void nova_wasi_setArgs(void* wasiPtr, const char** args, int count) {
    if (!wasiPtr) return;
    NovaWASI* wasi = (NovaWASI*)wasiPtr;
    wasi->args.clear();
    for (int i = 0; i < count; i++) {
        if (args[i]) wasi->args.push_back(args[i]);
    }
}

void nova_wasi_setEnv(void* wasiPtr, const char* key, const char* value) {
    if (!wasiPtr || !key) return;
    NovaWASI* wasi = (NovaWASI*)wasiPtr;
    wasi->env[key] = value ? value : "";
}

void nova_wasi_addPreopen(void* wasiPtr, const char* guestPath, const char* hostPath) {
    if (!wasiPtr || !guestPath || !hostPath) return;
    NovaWASI* wasi = (NovaWASI*)wasiPtr;

    NovaWASIPreopen preopen;
    preopen.guestPath = guestPath;
    preopen.hostPath = hostPath;
    preopen.fd = wasi->nextFd++;
    wasi->preopens.push_back(preopen);
    wasi->fdPaths[preopen.fd] = hostPath;
}

// ============================================================================
// WASI Import Object
// ============================================================================

const char* nova_wasi_getImportObject(void* wasiPtr) {
    if (!wasiPtr) return "{}";

    static thread_local std::string result;
    std::ostringstream json;
    json << "{\"wasi_snapshot_preview1\":{";
    json << "\"args_get\":\"[native]\",";
    json << "\"args_sizes_get\":\"[native]\",";
    json << "\"environ_get\":\"[native]\",";
    json << "\"environ_sizes_get\":\"[native]\",";
    json << "\"clock_res_get\":\"[native]\",";
    json << "\"clock_time_get\":\"[native]\",";
    json << "\"fd_advise\":\"[native]\",";
    json << "\"fd_allocate\":\"[native]\",";
    json << "\"fd_close\":\"[native]\",";
    json << "\"fd_datasync\":\"[native]\",";
    json << "\"fd_fdstat_get\":\"[native]\",";
    json << "\"fd_fdstat_set_flags\":\"[native]\",";
    json << "\"fd_filestat_get\":\"[native]\",";
    json << "\"fd_filestat_set_size\":\"[native]\",";
    json << "\"fd_filestat_set_times\":\"[native]\",";
    json << "\"fd_pread\":\"[native]\",";
    json << "\"fd_prestat_get\":\"[native]\",";
    json << "\"fd_prestat_dir_name\":\"[native]\",";
    json << "\"fd_pwrite\":\"[native]\",";
    json << "\"fd_read\":\"[native]\",";
    json << "\"fd_readdir\":\"[native]\",";
    json << "\"fd_renumber\":\"[native]\",";
    json << "\"fd_seek\":\"[native]\",";
    json << "\"fd_sync\":\"[native]\",";
    json << "\"fd_tell\":\"[native]\",";
    json << "\"fd_write\":\"[native]\",";
    json << "\"path_create_directory\":\"[native]\",";
    json << "\"path_filestat_get\":\"[native]\",";
    json << "\"path_filestat_set_times\":\"[native]\",";
    json << "\"path_link\":\"[native]\",";
    json << "\"path_open\":\"[native]\",";
    json << "\"path_readlink\":\"[native]\",";
    json << "\"path_remove_directory\":\"[native]\",";
    json << "\"path_rename\":\"[native]\",";
    json << "\"path_symlink\":\"[native]\",";
    json << "\"path_unlink_file\":\"[native]\",";
    json << "\"poll_oneoff\":\"[native]\",";
    json << "\"proc_exit\":\"[native]\",";
    json << "\"proc_raise\":\"[native]\",";
    json << "\"random_get\":\"[native]\",";
    json << "\"sched_yield\":\"[native]\",";
    json << "\"sock_accept\":\"[native]\",";
    json << "\"sock_recv\":\"[native]\",";
    json << "\"sock_send\":\"[native]\",";
    json << "\"sock_shutdown\":\"[native]\"";
    json << "}}";

    result = json.str();
    return result.c_str();
}

// ============================================================================
// WASI Start/Initialize
// ============================================================================

int nova_wasi_start(void* wasiPtr, void* wasmInstance) {
    if (!wasiPtr) return WASI_EINVAL;
    NovaWASI* wasi = (NovaWASI*)wasiPtr;

    if (wasi->started) return WASI_EALREADY;
    wasi->started = true;

    // In real implementation, call _start export of WASM instance
    (void)wasmInstance;

    return wasi->exitCode;
}

int nova_wasi_initialize(void* wasiPtr, void* wasmInstance) {
    if (!wasiPtr) return WASI_EINVAL;
    NovaWASI* wasi = (NovaWASI*)wasiPtr;

    if (wasi->started) return WASI_EALREADY;

    // In real implementation, call _initialize export of WASM instance
    (void)wasmInstance;

    return WASI_ESUCCESS;
}

// ============================================================================
// WASI System Calls - Args
// ============================================================================

int nova_wasi_args_sizes_get(void* wasiPtr, int* argc, int* argvBufSize) {
    if (!wasiPtr || !argc || !argvBufSize) return WASI_EINVAL;
    NovaWASI* wasi = (NovaWASI*)wasiPtr;

    *argc = (int)wasi->args.size();
    *argvBufSize = 0;
    for (const auto& arg : wasi->args) {
        *argvBufSize += (int)arg.length() + 1;  // +1 for null terminator
    }

    return WASI_ESUCCESS;
}

int nova_wasi_args_get(void* wasiPtr, char** argv, char* argvBuf) {
    if (!wasiPtr || !argv || !argvBuf) return WASI_EINVAL;
    NovaWASI* wasi = (NovaWASI*)wasiPtr;

    char* ptr = argvBuf;
    for (size_t i = 0; i < wasi->args.size(); i++) {
        argv[i] = ptr;
        strcpy(ptr, wasi->args[i].c_str());
        ptr += wasi->args[i].length() + 1;
    }

    return WASI_ESUCCESS;
}

// ============================================================================
// WASI System Calls - Environment
// ============================================================================

int nova_wasi_environ_sizes_get(void* wasiPtr, int* environCount, int* environBufSize) {
    if (!wasiPtr || !environCount || !environBufSize) return WASI_EINVAL;
    NovaWASI* wasi = (NovaWASI*)wasiPtr;

    *environCount = (int)wasi->env.size();
    *environBufSize = 0;
    for (const auto& pair : wasi->env) {
        *environBufSize += (int)pair.first.length() + 1 + (int)pair.second.length() + 1;
    }

    return WASI_ESUCCESS;
}

int nova_wasi_environ_get(void* wasiPtr, char** environ, char* environBuf) {
    if (!wasiPtr || !environ || !environBuf) return WASI_EINVAL;
    NovaWASI* wasi = (NovaWASI*)wasiPtr;

    char* ptr = environBuf;
    int i = 0;
    for (const auto& pair : wasi->env) {
        environ[i++] = ptr;
        sprintf(ptr, "%s=%s", pair.first.c_str(), pair.second.c_str());
        ptr += pair.first.length() + 1 + pair.second.length() + 1;
    }

    return WASI_ESUCCESS;
}

// ============================================================================
// WASI System Calls - Clock
// ============================================================================

int nova_wasi_clock_res_get(int clockId, int64_t* resolution) {
    if (!resolution) return WASI_EINVAL;

    switch (clockId) {
        case WASI_CLOCK_REALTIME:
        case WASI_CLOCK_MONOTONIC:
            *resolution = 1;  // nanosecond resolution
            break;
        case WASI_CLOCK_PROCESS_CPUTIME_ID:
        case WASI_CLOCK_THREAD_CPUTIME_ID:
            *resolution = 1000;  // microsecond resolution
            break;
        default:
            return WASI_EINVAL;
    }

    return WASI_ESUCCESS;
}

int nova_wasi_clock_time_get(int clockId, int64_t precision, int64_t* time) {
    if (!time) return WASI_EINVAL;
    (void)precision;

    switch (clockId) {
        case WASI_CLOCK_REALTIME: {
            auto now = std::chrono::system_clock::now();
            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                now.time_since_epoch()).count();
            *time = ns;
            break;
        }
        case WASI_CLOCK_MONOTONIC: {
            auto now = std::chrono::steady_clock::now();
            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                now.time_since_epoch()).count();
            *time = ns;
            break;
        }
        default:
            return WASI_EINVAL;
    }

    return WASI_ESUCCESS;
}

// ============================================================================
// WASI System Calls - File Descriptors
// ============================================================================

int nova_wasi_fd_close(void* wasiPtr, int fd) {
    if (!wasiPtr) return WASI_EINVAL;
    NovaWASI* wasi = (NovaWASI*)wasiPtr;

    if (fd < 3) return WASI_EBADF;  // Don't close stdin/stdout/stderr

    wasi->fdPaths.erase(fd);
#ifdef _WIN32
    return _close(fd) == 0 ? WASI_ESUCCESS : WASI_EIO;
#else
    return close(fd) == 0 ? WASI_ESUCCESS : WASI_EIO;
#endif
}

int nova_wasi_fd_read(void* wasiPtr, int fd, void* iovs, int iovsLen, int* nread) {
    if (!wasiPtr || !iovs || !nread) return WASI_EINVAL;
    (void)wasiPtr;

    // Simplified: read into first iov only
    WasiIovec* iov = (WasiIovec*)iovs;

    if (iovsLen < 1 || !iov[0].buf) return WASI_EINVAL;

#ifdef _WIN32
    int result = _read(fd, iov[0].buf, (unsigned int)iov[0].len);
#else
    ssize_t result = read(fd, iov[0].buf, iov[0].len);
#endif

    if (result < 0) return WASI_EIO;
    *nread = (int)result;
    return WASI_ESUCCESS;
}

int nova_wasi_fd_write(void* wasiPtr, int fd, const void* iovs, int iovsLen, int* nwritten) {
    if (!wasiPtr || !iovs || !nwritten) return WASI_EINVAL;
    (void)wasiPtr;

    WasiCiovec* iov = (WasiCiovec*)iovs;

    if (iovsLen < 1 || !iov[0].buf) return WASI_EINVAL;

#ifdef _WIN32
    int result = _write(fd, iov[0].buf, (unsigned int)iov[0].len);
#else
    ssize_t result = write(fd, iov[0].buf, iov[0].len);
#endif

    if (result < 0) return WASI_EIO;
    *nwritten = (int)result;
    return WASI_ESUCCESS;
}

int nova_wasi_fd_seek(void* wasiPtr, int fd, int64_t offset, int whence, int64_t* newoffset) {
    if (!wasiPtr || !newoffset) return WASI_EINVAL;
    (void)wasiPtr;

#ifdef _WIN32
    int64_t result = _lseeki64(fd, offset, whence);
#else
    off_t result = lseek(fd, offset, whence);
#endif

    if (result < 0) return WASI_EIO;
    *newoffset = result;
    return WASI_ESUCCESS;
}

int nova_wasi_fd_tell(void* wasiPtr, int fd, int64_t* offset) {
    return nova_wasi_fd_seek(wasiPtr, fd, 0, SEEK_CUR, offset);
}

int nova_wasi_fd_sync(void* wasiPtr, int fd) {
    if (!wasiPtr) return WASI_EINVAL;
    (void)wasiPtr;

#ifdef _WIN32
    return _commit(fd) == 0 ? WASI_ESUCCESS : WASI_EIO;
#else
    return fsync(fd) == 0 ? WASI_ESUCCESS : WASI_EIO;
#endif
}

int nova_wasi_fd_datasync(void* wasiPtr, int fd) {
    return nova_wasi_fd_sync(wasiPtr, fd);
}

// ============================================================================
// WASI System Calls - Prestat
// ============================================================================

int nova_wasi_fd_prestat_get(void* wasiPtr, int fd, int* prType, int* prNameLen) {
    if (!wasiPtr || !prType || !prNameLen) return WASI_EINVAL;
    NovaWASI* wasi = (NovaWASI*)wasiPtr;

    for (const auto& preopen : wasi->preopens) {
        if (preopen.fd == fd) {
            *prType = 0;  // PREOPENTYPE_DIR
            *prNameLen = (int)preopen.guestPath.length();
            return WASI_ESUCCESS;
        }
    }

    return WASI_EBADF;
}

int nova_wasi_fd_prestat_dir_name(void* wasiPtr, int fd, char* path, int pathLen) {
    if (!wasiPtr || !path) return WASI_EINVAL;
    NovaWASI* wasi = (NovaWASI*)wasiPtr;

    for (const auto& preopen : wasi->preopens) {
        if (preopen.fd == fd) {
            if ((int)preopen.guestPath.length() > pathLen) return WASI_EINVAL;
            strcpy(path, preopen.guestPath.c_str());
            return WASI_ESUCCESS;
        }
    }

    return WASI_EBADF;
}

// ============================================================================
// WASI System Calls - Path Operations
// ============================================================================

int nova_wasi_path_open(void* wasiPtr, int dirfd, int lookupFlags,
                         const char* path, int pathLen,
                         int oflags, int64_t fsRightsBase, int64_t fsRightsInheriting,
                         int fdflags, int* fd) {
    if (!wasiPtr || !path || !fd) return WASI_EINVAL;
    NovaWASI* wasi = (NovaWASI*)wasiPtr;
    (void)lookupFlags; (void)fsRightsBase; (void)fsRightsInheriting;

    std::string fullPath;
    auto it = wasi->fdPaths.find(dirfd);
    if (it != wasi->fdPaths.end()) {
        fullPath = it->second + "/" + std::string(path, pathLen);
    } else {
        fullPath = std::string(path, pathLen);
    }

    int flags = 0;
#ifdef _WIN32
    flags |= _O_BINARY;
    if (oflags & 1) flags |= _O_CREAT;
    if (oflags & 4) flags |= _O_TRUNC;
    if (fdflags & WASI_FDFLAG_APPEND) flags |= _O_APPEND;
    *fd = _open(fullPath.c_str(), flags, 0644);
#else
    if (oflags & 1) flags |= O_CREAT;
    if (oflags & 4) flags |= O_TRUNC;
    if (fdflags & WASI_FDFLAG_APPEND) flags |= O_APPEND;
    *fd = open(fullPath.c_str(), flags, 0644);
#endif

    if (*fd < 0) return WASI_ENOENT;

    wasi->fdPaths[*fd] = fullPath;
    return WASI_ESUCCESS;
}

int nova_wasi_path_create_directory(void* wasiPtr, int fd, const char* path, int pathLen) {
    if (!wasiPtr || !path) return WASI_EINVAL;
    NovaWASI* wasi = (NovaWASI*)wasiPtr;

    std::string fullPath;
    auto it = wasi->fdPaths.find(fd);
    if (it != wasi->fdPaths.end()) {
        fullPath = it->second + "/" + std::string(path, pathLen);
    } else {
        return WASI_EBADF;
    }

#ifdef _WIN32
    return CreateDirectoryA(fullPath.c_str(), NULL) ? WASI_ESUCCESS : WASI_EIO;
#else
    return mkdir(fullPath.c_str(), 0755) == 0 ? WASI_ESUCCESS : WASI_EIO;
#endif
}

int nova_wasi_path_remove_directory(void* wasiPtr, int fd, const char* path, int pathLen) {
    if (!wasiPtr || !path) return WASI_EINVAL;
    NovaWASI* wasi = (NovaWASI*)wasiPtr;

    std::string fullPath;
    auto it = wasi->fdPaths.find(fd);
    if (it != wasi->fdPaths.end()) {
        fullPath = it->second + "/" + std::string(path, pathLen);
    } else {
        return WASI_EBADF;
    }

#ifdef _WIN32
    return RemoveDirectoryA(fullPath.c_str()) ? WASI_ESUCCESS : WASI_EIO;
#else
    return rmdir(fullPath.c_str()) == 0 ? WASI_ESUCCESS : WASI_EIO;
#endif
}

int nova_wasi_path_unlink_file(void* wasiPtr, int fd, const char* path, int pathLen) {
    if (!wasiPtr || !path) return WASI_EINVAL;
    NovaWASI* wasi = (NovaWASI*)wasiPtr;

    std::string fullPath;
    auto it = wasi->fdPaths.find(fd);
    if (it != wasi->fdPaths.end()) {
        fullPath = it->second + "/" + std::string(path, pathLen);
    } else {
        return WASI_EBADF;
    }

#ifdef _WIN32
    return DeleteFileA(fullPath.c_str()) ? WASI_ESUCCESS : WASI_EIO;
#else
    return unlink(fullPath.c_str()) == 0 ? WASI_ESUCCESS : WASI_EIO;
#endif
}

int nova_wasi_path_rename(void* wasiPtr, int oldFd, const char* oldPath, int oldPathLen,
                           int newFd, const char* newPath, int newPathLen) {
    if (!wasiPtr || !oldPath || !newPath) return WASI_EINVAL;
    NovaWASI* wasi = (NovaWASI*)wasiPtr;

    std::string oldFullPath, newFullPath;
    auto it = wasi->fdPaths.find(oldFd);
    if (it != wasi->fdPaths.end()) {
        oldFullPath = it->second + "/" + std::string(oldPath, oldPathLen);
    } else {
        return WASI_EBADF;
    }

    it = wasi->fdPaths.find(newFd);
    if (it != wasi->fdPaths.end()) {
        newFullPath = it->second + "/" + std::string(newPath, newPathLen);
    } else {
        return WASI_EBADF;
    }

    return rename(oldFullPath.c_str(), newFullPath.c_str()) == 0 ? WASI_ESUCCESS : WASI_EIO;
}

// ============================================================================
// WASI System Calls - Random
// ============================================================================

int nova_wasi_random_get(void* buf, int bufLen) {
    if (!buf || bufLen <= 0) return WASI_EINVAL;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned int> dist(0, 255);

    uint8_t* bytes = (uint8_t*)buf;
    for (int i = 0; i < bufLen; i++) {
        bytes[i] = static_cast<uint8_t>(dist(gen));
    }

    return WASI_ESUCCESS;
}

// ============================================================================
// WASI System Calls - Process
// ============================================================================

void nova_wasi_proc_exit(void* wasiPtr, int exitCode) {
    if (wasiPtr) {
        NovaWASI* wasi = (NovaWASI*)wasiPtr;
        wasi->exitCode = exitCode;
        if (!wasi->returnOnExit) {
            exit(exitCode);
        }
    }
}

int nova_wasi_sched_yield() {
#ifdef _WIN32
    SwitchToThread();
#else
    sched_yield();
#endif
    return WASI_ESUCCESS;
}

// ============================================================================
// WASI Properties
// ============================================================================

const char* nova_wasi_getVersion(void* wasiPtr) {
    if (!wasiPtr) return "preview1";
    return ((NovaWASI*)wasiPtr)->version.c_str();
}

int nova_wasi_getExitCode(void* wasiPtr) {
    if (!wasiPtr) return 0;
    return ((NovaWASI*)wasiPtr)->exitCode;
}

int nova_wasi_isStarted(void* wasiPtr) {
    if (!wasiPtr) return 0;
    return ((NovaWASI*)wasiPtr)->started ? 1 : 0;
}

// ============================================================================
// WASI Cleanup
// ============================================================================

void nova_wasi_free(void* wasiPtr) {
    if (wasiPtr) delete (NovaWASI*)wasiPtr;
}

// ============================================================================
// Error String
// ============================================================================

const char* nova_wasi_strerror(int error) {
    switch (error) {
        case WASI_ESUCCESS: return "Success";
        case WASI_E2BIG: return "Argument list too long";
        case WASI_EACCES: return "Permission denied";
        case WASI_EBADF: return "Bad file descriptor";
        case WASI_EBUSY: return "Device or resource busy";
        case WASI_EEXIST: return "File exists";
        case WASI_EINVAL: return "Invalid argument";
        case WASI_EIO: return "I/O error";
        case WASI_EISDIR: return "Is a directory";
        case WASI_ENOENT: return "No such file or directory";
        case WASI_ENOMEM: return "Out of memory";
        case WASI_ENOSYS: return "Function not implemented";
        case WASI_ENOTDIR: return "Not a directory";
        case WASI_EPERM: return "Operation not permitted";
        default: return "Unknown error";
    }
}

} // extern "C"
