// Nova Process Module - Node.js compatible process API
// Provides process information and control

#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <functional>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#include <direct.h>
#include <io.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#include <dlfcn.h>
#endif

// Process start time for uptime calculation
static auto processStartTime = std::chrono::steady_clock::now();
static auto processStartHighRes = std::chrono::high_resolution_clock::now();

// Exit code
static int processExitCode = 0;

// Warning listeners
static std::vector<std::function<void(const char*, const char*, const char*)>> warningListeners;

// Event listeners
static std::map<std::string, std::vector<std::function<void()>>> eventListeners;

// Uncaught exception callback
static std::function<void(void*)> uncaughtExceptionCallback = nullptr;

// Source maps enabled
static bool sourceMapsEnabled = false;

// Process title
static std::string processTitle = "nova";

// Command line arguments storage
static std::vector<std::string> storedArgv;
static std::vector<std::string> storedExecArgv;

// IPC Channel support (for cluster/worker communication)
static bool ipcConnected = false;
static int ipcChannelFd = -1;
static void* ipcChannelPtr = nullptr;

// Message queue for IPC
static std::vector<std::string> ipcMessageQueue;
static std::function<void(const char*)> ipcMessageCallback = nullptr;

extern "C" {

// ============================================================================
// Process Properties
// ============================================================================

// process.arch - CPU architecture
const char* nova_process_arch() {
#if defined(__x86_64__) || defined(_M_X64)
    return "x64";
#elif defined(__i386__) || defined(_M_IX86)
    return "ia32";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "arm64";
#elif defined(__arm__) || defined(_M_ARM)
    return "arm";
#elif defined(__mips64)
    return "mips64el";
#elif defined(__mips__)
    return "mipsel";
#elif defined(__ppc64__)
    return "ppc64";
#elif defined(__s390x__)
    return "s390x";
#else
    return "unknown";
#endif
}

// process.platform
const char* nova_process_platform() {
#ifdef _WIN32
    return "win32";
#elif defined(__APPLE__)
    return "darwin";
#elif defined(__linux__)
    return "linux";
#elif defined(__FreeBSD__)
    return "freebsd";
#elif defined(__OpenBSD__)
    return "openbsd";
#elif defined(__sun)
    return "sunos";
#elif defined(_AIX)
    return "aix";
#else
    return "unknown";
#endif
}

// process.pid
int nova_process_pid() {
#ifdef _WIN32
    return _getpid();
#else
    return getpid();
#endif
}

// process.ppid - Parent process ID
int nova_process_ppid() {
#ifdef _WIN32
    // Windows doesn't have a direct ppid, return 0
    return 0;
#else
    return getppid();
#endif
}

// process.version
const char* nova_process_version() {
    return "v20.0.0";  // Nova reports Node.js compatible version
}

// process.versions - Returns version info as JSON string
const char* nova_process_versions() {
    static std::string versionsJson = R"({
  "node": "20.0.0",
  "nova": "1.0.0",
  "v8": "11.3.244.8",
  "uv": "1.44.2",
  "zlib": "1.2.13",
  "ares": "1.19.0",
  "modules": "115",
  "nghttp2": "1.52.0",
  "napi": "8",
  "llhttp": "8.1.0",
  "openssl": "3.0.8",
  "cldr": "42.0",
  "icu": "72.1",
  "tz": "2022g",
  "unicode": "15.0"
})";
    return versionsJson.c_str();
}

// process.argv - Command line arguments
const char** nova_process_argv(int* count) {
    static std::vector<const char*> argvPtrs;
    argvPtrs.clear();

    for (const auto& arg : storedArgv) {
        argvPtrs.push_back(arg.c_str());
    }

    *count = static_cast<int>(argvPtrs.size());
    return argvPtrs.data();
}

// process.argv0 - Original argv[0]
const char* nova_process_argv0() {
    if (storedArgv.empty()) {
        return "nova";
    }
    return storedArgv[0].c_str();
}

// Set argv (called during initialization)
void nova_process_setArgv(const char** argv, int argc) {
    storedArgv.clear();
    for (int i = 0; i < argc; i++) {
        storedArgv.push_back(argv[i]);
    }
}

// process.execArgv - Node-specific command line options
const char** nova_process_execArgv(int* count) {
    static std::vector<const char*> execArgvPtrs;
    execArgvPtrs.clear();

    for (const auto& arg : storedExecArgv) {
        execArgvPtrs.push_back(arg.c_str());
    }

    *count = static_cast<int>(execArgvPtrs.size());
    return execArgvPtrs.data();
}

// process.execPath
const char* nova_process_execPath() {
    static std::string execPath;

#ifdef _WIN32
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    execPath = path;
#else
    char path[1024];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        execPath = path;
    } else {
        execPath = "/usr/local/bin/nova";
    }
#endif

    return execPath.c_str();
}

// process.cwd()
const char* nova_process_cwd() {
    static std::string cwdPath;

#ifdef _WIN32
    char path[MAX_PATH];
    _getcwd(path, MAX_PATH);
    cwdPath = path;
#else
    char path[1024];
    if (getcwd(path, sizeof(path))) {
        cwdPath = path;
    }
#endif

    return cwdPath.c_str();
}

// process.chdir(directory)
bool nova_process_chdir(const char* directory) {
#ifdef _WIN32
    return _chdir(directory) == 0;
#else
    return chdir(directory) == 0;
#endif
}

// process.env - Get environment variable
const char* nova_process_env_get(const char* name) {
    return std::getenv(name);
}

// process.env - Set environment variable
bool nova_process_env_set(const char* name, const char* value) {
#ifdef _WIN32
    return _putenv_s(name, value) == 0;
#else
    return setenv(name, value, 1) == 0;
#endif
}

// process.env - Delete environment variable
bool nova_process_env_delete(const char* name) {
#ifdef _WIN32
    return _putenv_s(name, "") == 0;
#else
    return unsetenv(name) == 0;
#endif
}

// process.env - Get all environment variables as JSON
const char* nova_process_env_all() {
    static std::string envJson;
    envJson = "{";

#ifdef _WIN32
    // Use wide char version and convert to UTF-8
    wchar_t* envStrings = GetEnvironmentStringsW();
    if (envStrings) {
        wchar_t* current = envStrings;
        bool first = true;
        while (*current) {
            // Convert wide string to UTF-8
            int len = WideCharToMultiByte(CP_UTF8, 0, current, -1, nullptr, 0, nullptr, nullptr);
            std::string entry(len - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, current, -1, &entry[0], len, nullptr, nullptr);

            size_t eqPos = entry.find('=');
            if (eqPos != std::string::npos && eqPos > 0) {
                if (!first) envJson += ",";
                first = false;

                std::string key = entry.substr(0, eqPos);
                std::string value = entry.substr(eqPos + 1);

                // Escape quotes in key and value
                auto escape = [](const std::string& s) {
                    std::string result;
                    for (char c : s) {
                        if (c == '"') result += "\\\"";
                        else if (c == '\\') result += "\\\\";
                        else if (c == '\n') result += "\\n";
                        else if (c == '\r') result += "\\r";
                        else if (c == '\t') result += "\\t";
                        else result += c;
                    }
                    return result;
                };

                envJson += "\"" + escape(key) + "\":\"" + escape(value) + "\"";
            }
            current += wcslen(current) + 1;
        }
        FreeEnvironmentStringsW(envStrings);
    }
#else
    extern char** environ;
    bool first = true;
    for (char** env = environ; *env; ++env) {
        std::string entry(*env);
        size_t eqPos = entry.find('=');
        if (eqPos != std::string::npos) {
            if (!first) envJson += ",";
            first = false;

            std::string key = entry.substr(0, eqPos);
            std::string value = entry.substr(eqPos + 1);

            auto escape = [](const std::string& s) {
                std::string result;
                for (char c : s) {
                    if (c == '"') result += "\\\"";
                    else if (c == '\\') result += "\\\\";
                    else if (c == '\n') result += "\\n";
                    else if (c == '\r') result += "\\r";
                    else if (c == '\t') result += "\\t";
                    else result += c;
                }
                return result;
            };

            envJson += "\"" + escape(key) + "\":\"" + escape(value) + "\"";
        }
    }
#endif

    envJson += "}";
    return envJson.c_str();
}

// process.title - Get
const char* nova_process_title_get() {
    return processTitle.c_str();
}

// process.title - Set
void nova_process_title_set(const char* title) {
    processTitle = title;
#ifdef _WIN32
    SetConsoleTitleA(title);
#endif
}

// process.debugPort
int nova_process_debugPort() {
    return 9229;  // Default Node.js debug port
}

// process.exitCode - Get
int nova_process_exitCode_get() {
    return processExitCode;
}

// process.exitCode - Set
void nova_process_exitCode_set(int code) {
    processExitCode = code;
}

// process.connected (for IPC)
bool nova_process_connected() {
    return ipcConnected;
}

// process.channel (for IPC)
void* nova_process_channel() {
    return ipcChannelPtr;
}

// Initialize IPC channel (called when NODE_CHANNEL_FD is set)
void nova_process_initIPC() {
    // Check for NODE_CHANNEL_FD environment variable (set by parent process)
    const char* channelFd = std::getenv("NODE_CHANNEL_FD");
    if (channelFd) {
        ipcChannelFd = std::atoi(channelFd);
        if (ipcChannelFd >= 0) {
            ipcConnected = true;
            ipcChannelPtr = (void*)(intptr_t)ipcChannelFd;
        }
    }
}

// Set IPC message handler
void nova_process_onMessage(void (*callback)(const char*)) {
    ipcMessageCallback = callback;
}

// Check if IPC channel exists
bool nova_process_hasChannel() {
    return ipcChannelFd >= 0;
}

// ============================================================================
// Process Methods
// ============================================================================

// process.exit([code])
void nova_process_exit(int code) {
    processExitCode = code;
    std::exit(code);
}

// process.abort()
void nova_process_abort() {
    std::abort();
}

// process.kill(pid, signal)
bool nova_process_kill(int pid, [[maybe_unused]] const char* signal) {
#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess) {
        bool result = TerminateProcess(hProcess, 1) != 0;
        CloseHandle(hProcess);
        return result;
    }
    return false;
#else
    int sig = SIGTERM;  // Default
    if (signal) {
        if (strcmp(signal, "SIGKILL") == 0) sig = SIGKILL;
        else if (strcmp(signal, "SIGINT") == 0) sig = SIGINT;
        else if (strcmp(signal, "SIGHUP") == 0) sig = SIGHUP;
        else if (strcmp(signal, "SIGUSR1") == 0) sig = SIGUSR1;
        else if (strcmp(signal, "SIGUSR2") == 0) sig = SIGUSR2;
        else if (strcmp(signal, "SIGTERM") == 0) sig = SIGTERM;
        else if (strcmp(signal, "SIGSTOP") == 0) sig = SIGSTOP;
        else if (strcmp(signal, "SIGCONT") == 0) sig = SIGCONT;
    }
    return kill(pid, sig) == 0;
#endif
}

// process.uptime() - Returns uptime in seconds
double nova_process_uptime() {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - processStartTime);
    return duration.count() / 1000.0;
}

// process.hrtime() - High resolution time
void nova_process_hrtime(int64_t* seconds, int64_t* nanoseconds) {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    auto sec = std::chrono::duration_cast<std::chrono::seconds>(duration);
    auto nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(duration - sec);

    *seconds = sec.count();
    *nanoseconds = nsec.count();
}

// process.hrtime(previousTime) - Difference from previous
void nova_process_hrtime_diff(int64_t prevSec, int64_t prevNsec, int64_t* seconds, int64_t* nanoseconds) {
    int64_t nowSec, nowNsec;
    nova_process_hrtime(&nowSec, &nowNsec);

    int64_t diffNsec = nowNsec - prevNsec;
    int64_t diffSec = nowSec - prevSec;

    if (diffNsec < 0) {
        diffSec--;
        diffNsec += 1000000000LL;
    }

    *seconds = diffSec;
    *nanoseconds = diffNsec;
}

// process.hrtime.bigint() - Returns nanoseconds as int64
int64_t nova_process_hrtime_bigint() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

// ============================================================================
// Memory Usage
// ============================================================================

struct MemoryUsage {
    int64_t rss;
    int64_t heapTotal;
    int64_t heapUsed;
    int64_t external;
    int64_t arrayBuffers;
};

// process.memoryUsage()
void* nova_process_memoryUsage() {
    auto* usage = new MemoryUsage();

#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        usage->rss = pmc.WorkingSetSize;
        usage->heapTotal = pmc.PrivateUsage;
        usage->heapUsed = pmc.PrivateUsage;
    }
#else
    // Read from /proc/self/statm on Linux
    FILE* f = fopen("/proc/self/statm", "r");
    if (f) {
        long pages = 0;
        if (fscanf(f, "%ld", &pages) == 1) {
            usage->rss = pages * sysconf(_SC_PAGESIZE);
        }
        fclose(f);
    }

    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) == 0) {
        usage->rss = ru.ru_maxrss * 1024;  // KB to bytes
    }
#endif

    usage->external = 0;
    usage->arrayBuffers = 0;

    return usage;
}

int64_t nova_process_memoryUsage_rss(void* usage) {
    return static_cast<MemoryUsage*>(usage)->rss;
}

int64_t nova_process_memoryUsage_heapTotal(void* usage) {
    return static_cast<MemoryUsage*>(usage)->heapTotal;
}

int64_t nova_process_memoryUsage_heapUsed(void* usage) {
    return static_cast<MemoryUsage*>(usage)->heapUsed;
}

int64_t nova_process_memoryUsage_external(void* usage) {
    return static_cast<MemoryUsage*>(usage)->external;
}

int64_t nova_process_memoryUsage_arrayBuffers(void* usage) {
    return static_cast<MemoryUsage*>(usage)->arrayBuffers;
}

void nova_process_memoryUsage_free(void* usage) {
    delete static_cast<MemoryUsage*>(usage);
}

// process.memoryUsage.rss() - Quick RSS accessor
int64_t nova_process_memoryUsage_rss_quick() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
    return 0;
#else
    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) == 0) {
        return ru.ru_maxrss * 1024;
    }
    return 0;
#endif
}

// ============================================================================
// CPU Usage
// ============================================================================

struct CpuUsage {
    int64_t user;
    int64_t system;
};

// process.cpuUsage()
void* nova_process_cpuUsage() {
    auto* usage = new CpuUsage();

#ifdef _WIN32
    FILETIME createTime, exitTime, kernelTime, userTime;
    if (GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &kernelTime, &userTime)) {
        ULARGE_INTEGER kernel, user;
        kernel.LowPart = kernelTime.dwLowDateTime;
        kernel.HighPart = kernelTime.dwHighDateTime;
        user.LowPart = userTime.dwLowDateTime;
        user.HighPart = userTime.dwHighDateTime;

        // Convert 100-nanosecond intervals to microseconds
        usage->user = user.QuadPart / 10;
        usage->system = kernel.QuadPart / 10;
    }
#else
    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) == 0) {
        usage->user = ru.ru_utime.tv_sec * 1000000LL + ru.ru_utime.tv_usec;
        usage->system = ru.ru_stime.tv_sec * 1000000LL + ru.ru_stime.tv_usec;
    }
#endif

    return usage;
}

// process.cpuUsage(previousValue)
void* nova_process_cpuUsage_diff(void* prev) {
    auto* current = static_cast<CpuUsage*>(nova_process_cpuUsage());
    auto* previous = static_cast<CpuUsage*>(prev);

    if (previous) {
        current->user -= previous->user;
        current->system -= previous->system;
    }

    return current;
}

int64_t nova_process_cpuUsage_user(void* usage) {
    return static_cast<CpuUsage*>(usage)->user;
}

int64_t nova_process_cpuUsage_system(void* usage) {
    return static_cast<CpuUsage*>(usage)->system;
}

void nova_process_cpuUsage_free(void* usage) {
    delete static_cast<CpuUsage*>(usage);
}

// ============================================================================
// Resource Usage
// ============================================================================

struct ResourceUsage {
    int64_t userCPUTime;
    int64_t systemCPUTime;
    int64_t maxRSS;
    int64_t sharedMemorySize;
    int64_t unsharedDataSize;
    int64_t unsharedStackSize;
    int64_t minorPageFault;
    int64_t majorPageFault;
    int64_t swappedOut;
    int64_t fsRead;
    int64_t fsWrite;
    int64_t ipcSent;
    int64_t ipcReceived;
    int64_t signalsCount;
    int64_t voluntaryContextSwitches;
    int64_t involuntaryContextSwitches;
};

// process.resourceUsage()
void* nova_process_resourceUsage() {
    auto* usage = new ResourceUsage();
    memset(usage, 0, sizeof(ResourceUsage));

#ifdef _WIN32
    FILETIME createTime, exitTime, kernelTime, userTime;
    if (GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &kernelTime, &userTime)) {
        ULARGE_INTEGER kernel, user;
        kernel.LowPart = kernelTime.dwLowDateTime;
        kernel.HighPart = kernelTime.dwHighDateTime;
        user.LowPart = userTime.dwLowDateTime;
        user.HighPart = userTime.dwHighDateTime;

        usage->userCPUTime = user.QuadPart / 10;
        usage->systemCPUTime = kernel.QuadPart / 10;
    }

    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        usage->maxRSS = pmc.PeakWorkingSetSize;
        usage->majorPageFault = pmc.PageFaultCount;
    }
#else
    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) == 0) {
        usage->userCPUTime = ru.ru_utime.tv_sec * 1000000LL + ru.ru_utime.tv_usec;
        usage->systemCPUTime = ru.ru_stime.tv_sec * 1000000LL + ru.ru_stime.tv_usec;
        usage->maxRSS = ru.ru_maxrss * 1024;
        usage->sharedMemorySize = ru.ru_ixrss;
        usage->unsharedDataSize = ru.ru_idrss;
        usage->unsharedStackSize = ru.ru_isrss;
        usage->minorPageFault = ru.ru_minflt;
        usage->majorPageFault = ru.ru_majflt;
        usage->swappedOut = ru.ru_nswap;
        usage->fsRead = ru.ru_inblock;
        usage->fsWrite = ru.ru_oublock;
        usage->ipcSent = ru.ru_msgsnd;
        usage->ipcReceived = ru.ru_msgrcv;
        usage->signalsCount = ru.ru_nsignals;
        usage->voluntaryContextSwitches = ru.ru_nvcsw;
        usage->involuntaryContextSwitches = ru.ru_nivcsw;
    }
#endif

    return usage;
}

int64_t nova_process_resourceUsage_userCPUTime(void* usage) {
    return static_cast<ResourceUsage*>(usage)->userCPUTime;
}

int64_t nova_process_resourceUsage_systemCPUTime(void* usage) {
    return static_cast<ResourceUsage*>(usage)->systemCPUTime;
}

int64_t nova_process_resourceUsage_maxRSS(void* usage) {
    return static_cast<ResourceUsage*>(usage)->maxRSS;
}

int64_t nova_process_resourceUsage_sharedMemorySize(void* usage) {
    return static_cast<ResourceUsage*>(usage)->sharedMemorySize;
}

int64_t nova_process_resourceUsage_unsharedDataSize(void* usage) {
    return static_cast<ResourceUsage*>(usage)->unsharedDataSize;
}

int64_t nova_process_resourceUsage_unsharedStackSize(void* usage) {
    return static_cast<ResourceUsage*>(usage)->unsharedStackSize;
}

int64_t nova_process_resourceUsage_minorPageFault(void* usage) {
    return static_cast<ResourceUsage*>(usage)->minorPageFault;
}

int64_t nova_process_resourceUsage_majorPageFault(void* usage) {
    return static_cast<ResourceUsage*>(usage)->majorPageFault;
}

int64_t nova_process_resourceUsage_swappedOut(void* usage) {
    return static_cast<ResourceUsage*>(usage)->swappedOut;
}

int64_t nova_process_resourceUsage_fsRead(void* usage) {
    return static_cast<ResourceUsage*>(usage)->fsRead;
}

int64_t nova_process_resourceUsage_fsWrite(void* usage) {
    return static_cast<ResourceUsage*>(usage)->fsWrite;
}

int64_t nova_process_resourceUsage_ipcSent(void* usage) {
    return static_cast<ResourceUsage*>(usage)->ipcSent;
}

int64_t nova_process_resourceUsage_ipcReceived(void* usage) {
    return static_cast<ResourceUsage*>(usage)->ipcReceived;
}

int64_t nova_process_resourceUsage_signalsCount(void* usage) {
    return static_cast<ResourceUsage*>(usage)->signalsCount;
}

int64_t nova_process_resourceUsage_voluntaryContextSwitches(void* usage) {
    return static_cast<ResourceUsage*>(usage)->voluntaryContextSwitches;
}

int64_t nova_process_resourceUsage_involuntaryContextSwitches(void* usage) {
    return static_cast<ResourceUsage*>(usage)->involuntaryContextSwitches;
}

void nova_process_resourceUsage_free(void* usage) {
    delete static_cast<ResourceUsage*>(usage);
}

// ============================================================================
// User/Group IDs (Unix only, stubs for Windows)
// ============================================================================

// process.getuid()
int nova_process_getuid() {
#ifdef _WIN32
    return -1;  // Not supported on Windows
#else
    return getuid();
#endif
}

// process.geteuid()
int nova_process_geteuid() {
#ifdef _WIN32
    return -1;
#else
    return geteuid();
#endif
}

// process.getgid()
int nova_process_getgid() {
#ifdef _WIN32
    return -1;
#else
    return getgid();
#endif
}

// process.getegid()
int nova_process_getegid() {
#ifdef _WIN32
    return -1;
#else
    return getegid();
#endif
}

// process.setuid(id)
bool nova_process_setuid(int id) {
#ifdef _WIN32
    (void)id;
    return false;
#else
    return setuid(id) == 0;
#endif
}

// process.seteuid(id)
bool nova_process_seteuid(int id) {
#ifdef _WIN32
    (void)id;
    return false;
#else
    return seteuid(id) == 0;
#endif
}

// process.setgid(id)
bool nova_process_setgid(int id) {
#ifdef _WIN32
    (void)id;
    return false;
#else
    return setgid(id) == 0;
#endif
}

// process.setegid(id)
bool nova_process_setegid(int id) {
#ifdef _WIN32
    (void)id;
    return false;
#else
    return setegid(id) == 0;
#endif
}

// process.getgroups()
int* nova_process_getgroups(int* count) {
#ifdef _WIN32
    *count = 0;
    return nullptr;
#else
    int ngroups = getgroups(0, nullptr);
    if (ngroups <= 0) {
        *count = 0;
        return nullptr;
    }

    gid_t* groups = new gid_t[ngroups];
    ngroups = getgroups(ngroups, groups);

    int* result = new int[ngroups];
    for (int i = 0; i < ngroups; i++) {
        result[i] = groups[i];
    }
    delete[] groups;

    *count = ngroups;
    return result;
#endif
}

// process.setgroups(groups)
bool nova_process_setgroups(int* groups, int count) {
#ifdef _WIN32
    (void)groups;
    (void)count;
    return false;
#else
    gid_t* gids = new gid_t[count];
    for (int i = 0; i < count; i++) {
        gids[i] = groups[i];
    }
    bool result = setgroups(count, gids) == 0;
    delete[] gids;
    return result;
#endif
}

// process.initgroups(user, extraGroup)
bool nova_process_initgroups(const char* user, int extraGroup) {
#ifdef _WIN32
    (void)user;
    (void)extraGroup;
    return false;
#else
    return initgroups(user, extraGroup) == 0;
#endif
}

// ============================================================================
// File Mode Creation Mask
// ============================================================================

// process.umask()
int nova_process_umask_get() {
#ifdef _WIN32
    return 0;
#else
    mode_t current = umask(0);
    umask(current);
    return current;
#endif
}

// process.umask(mask)
int nova_process_umask_set(int mask) {
#ifdef _WIN32
    (void)mask;
    return 0;
#else
    return umask(mask);
#endif
}

// ============================================================================
// Warnings and Events
// ============================================================================

// process.emitWarning(warning, type, code)
void nova_process_emitWarning(const char* warning, const char* type, const char* code) {
    for (auto& listener : warningListeners) {
        listener(warning, type ? type : "Warning", code ? code : "");
    }

    // Default: print to stderr
    if (warningListeners.empty()) {
        fprintf(stderr, "(%s) %s: %s\n", code ? code : "", type ? type : "Warning", warning);
    }
}

// Add warning listener
void nova_process_onWarning(void (*callback)(const char*, const char*, const char*)) {
    warningListeners.push_back([callback](const char* w, const char* t, const char* c) {
        callback(w, t, c);
    });
}

// process.on(event, listener)
void nova_process_on(const char* event, void (*callback)()) {
    eventListeners[event].push_back(callback);
}

// process.once(event, listener)
void nova_process_once(const char* event, void (*callback)()) {
    // Simple implementation - will be called once then removed
    nova_process_on(event, callback);
}

// process.off(event, listener) - Remove event listener
void nova_process_off(const char* event, void (*callback)()) {
    auto& listeners = eventListeners[event];
    // Remove matching callback (simplified)
    (void)callback;
    listeners.clear();
}

// process.emit(event)
void nova_process_emit(const char* event) {
    auto it = eventListeners.find(event);
    if (it != eventListeners.end()) {
        for (auto& listener : it->second) {
            listener();
        }
    }
}

// process.listeners(event)
int nova_process_listenerCount(const char* event) {
    auto it = eventListeners.find(event);
    if (it != eventListeners.end()) {
        return static_cast<int>(it->second.size());
    }
    return 0;
}

// ============================================================================
// nextTick
// ============================================================================

static std::vector<std::function<void()>> nextTickQueue;

// process.nextTick(callback)
void nova_process_nextTick(void (*callback)()) {
    nextTickQueue.push_back(callback);
}

// Process nextTick queue (called by event loop)
void nova_process_runNextTicks() {
    while (!nextTickQueue.empty()) {
        auto callbacks = std::move(nextTickQueue);
        nextTickQueue.clear();

        for (auto& cb : callbacks) {
            cb();
        }
    }
}

// ============================================================================
// Uncaught Exception Handling
// ============================================================================

// process.setUncaughtExceptionCaptureCallback(fn)
void nova_process_setUncaughtExceptionCaptureCallback(void (*callback)(void*)) {
    if (callback) {
        uncaughtExceptionCallback = callback;
    } else {
        uncaughtExceptionCallback = nullptr;
    }
}

// process.hasUncaughtExceptionCaptureCallback()
bool nova_process_hasUncaughtExceptionCaptureCallback() {
    return uncaughtExceptionCallback != nullptr;
}

// ============================================================================
// Source Maps
// ============================================================================

// process.setSourceMapsEnabled(val)
void nova_process_setSourceMapsEnabled(bool enabled) {
    sourceMapsEnabled = enabled;
}

// Check if source maps enabled
bool nova_process_sourceMapsEnabled() {
    return sourceMapsEnabled;
}

// ============================================================================
// Standard IO Streams
// ============================================================================

// process.stdin
void* nova_process_stdin() {
    return stdin;
}

// process.stdout
void* nova_process_stdout() {
    return stdout;
}

// process.stderr
void* nova_process_stderr() {
    return stderr;
}

// process.stdin.isTTY
bool nova_process_stdin_isTTY() {
#ifdef _WIN32
    return _isatty(_fileno(stdin)) != 0;
#else
    return isatty(fileno(stdin)) != 0;
#endif
}

// process.stdout.isTTY
bool nova_process_stdout_isTTY() {
#ifdef _WIN32
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(fileno(stdout)) != 0;
#endif
}

// process.stderr.isTTY
bool nova_process_stderr_isTTY() {
#ifdef _WIN32
    return _isatty(_fileno(stderr)) != 0;
#else
    return isatty(fileno(stderr)) != 0;
#endif
}

// ============================================================================
// Release Info
// ============================================================================

// process.release
const char* nova_process_release() {
    static std::string releaseJson = R"({
  "name": "nova",
  "lts": "Hydrogen",
  "sourceUrl": "https://github.com/example/nova",
  "headersUrl": "https://github.com/example/nova/releases"
})";
    return releaseJson.c_str();
}

// ============================================================================
// Config
// ============================================================================

// process.config
const char* nova_process_config() {
    static std::string configJson = R"({
  "target_defaults": {
    "cflags": [],
    "default_configuration": "Release",
    "defines": [],
    "include_dirs": [],
    "libraries": []
  },
  "variables": {
    "asan": 0,
    "coverage": false,
    "debug_nghttp2": false,
    "enable_lto": false,
    "enable_pgo_generate": false,
    "enable_pgo_use": false,
    "force_dynamic_crt": 0,
    "host_arch": "x64",
    "icu_data_in": "../../deps/icu-tmp/icudt72l.dat",
    "icu_endianness": "l",
    "icu_gyp_path": "tools/icu/icu-generic.gyp",
    "icu_path": "deps/icu-small",
    "icu_small": false,
    "icu_ver_major": "72",
    "is_debug": 0,
    "llvm_version": "0.0",
    "napi_build_version": "8",
    "node_builtin_shareable_builtins": [],
    "node_byteorder": "little",
    "node_debug_lib": false,
    "node_enable_d8": false,
    "node_install_corepack": true,
    "node_install_npm": true,
    "node_library_files": [],
    "node_module_version": 115,
    "node_no_browser_globals": false,
    "node_prefix": "/",
    "node_release_urlbase": "",
    "node_shared": false,
    "node_shared_brotli": false,
    "node_shared_cares": false,
    "node_shared_http_parser": false,
    "node_shared_libuv": false,
    "node_shared_nghttp2": false,
    "node_shared_openssl": false,
    "node_shared_zlib": false,
    "node_tag": "",
    "node_target_type": "executable",
    "node_use_bundled_v8": true,
    "node_use_node_code_cache": true,
    "node_use_node_snapshot": true,
    "node_use_openssl": true,
    "node_use_v8_platform": true,
    "node_with_ltcg": true,
    "node_without_node_options": false,
    "openssl_is_fips": false,
    "openssl_quic": true,
    "ossfuzz": false,
    "shlib_suffix": "so.115",
    "target_arch": "x64",
    "v8_enable_31bit_smis_on_64bit_arch": 0,
    "v8_enable_gdbjit": 0,
    "v8_enable_hugepage": 0,
    "v8_enable_i18n_support": 1,
    "v8_enable_inspector": 1,
    "v8_enable_javascript_linkage": 0,
    "v8_enable_lite_mode": 0,
    "v8_enable_object_print": 1,
    "v8_enable_pointer_compression": 0,
    "v8_enable_shared_ro_heap": 1,
    "v8_enable_short_builtin_calls": 1,
    "v8_enable_webassembly": 1,
    "v8_no_strict_aliasing": 1,
    "v8_optimized_debug": 1,
    "v8_promise_internal_field_count": 1,
    "v8_random_seed": 0,
    "v8_trace_maps": 0,
    "v8_use_siphash": 1,
    "want_separate_host_toolset": 0
  }
})";
    return configJson.c_str();
}

// ============================================================================
// Features
// ============================================================================

// process.features
const char* nova_process_features() {
    static std::string featuresJson = R"({
  "inspector": true,
  "debug": false,
  "uv": true,
  "ipv6": true,
  "tls_alpn": true,
  "tls_sni": true,
  "tls_ocsp": true,
  "tls": true,
  "cached_builtins": true
})";
    return featuresJson.c_str();
}

// ============================================================================
// Report (diagnostic report)
// ============================================================================

// process.report.writeReport()
const char* nova_process_report_writeReport(const char* filename) {
    static std::string reportPath;

    if (filename) {
        reportPath = filename;
    } else {
        // Generate default filename
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        char buf[64];
        strftime(buf, sizeof(buf), "report.%Y%m%d.%H%M%S", localtime(&time));
        reportPath = std::string(buf) + "." + std::to_string(nova_process_pid()) + ".json";
    }

    // Generate a simple diagnostic report
    FILE* f = fopen(reportPath.c_str(), "w");
    if (f) {
        fprintf(f, "{\n");
        fprintf(f, "  \"header\": {\n");
        fprintf(f, "    \"reportVersion\": 3,\n");
        fprintf(f, "    \"nodejsVersion\": \"%s\",\n", nova_process_version());
        fprintf(f, "    \"arch\": \"%s\",\n", nova_process_arch());
        fprintf(f, "    \"platform\": \"%s\",\n", nova_process_platform());
        fprintf(f, "    \"componentVersions\": %s\n", nova_process_versions());
        fprintf(f, "  }\n");
        fprintf(f, "}\n");
        fclose(f);
    }

    return reportPath.c_str();
}

// process.report.getReport()
const char* nova_process_report_getReport() {
    static std::string report;
    report = "{\n";
    report += "  \"header\": {\n";
    report += "    \"reportVersion\": 3,\n";
    report += "    \"nodejsVersion\": \"" + std::string(nova_process_version()) + "\",\n";
    report += "    \"arch\": \"" + std::string(nova_process_arch()) + "\",\n";
    report += "    \"platform\": \"" + std::string(nova_process_platform()) + "\",\n";
    report += "    \"processId\": " + std::to_string(nova_process_pid()) + "\n";
    report += "  }\n";
    report += "}";
    return report.c_str();
}

// process.report.directory
static std::string reportDirectory = "";

const char* nova_process_report_directory_get() {
    return reportDirectory.c_str();
}

void nova_process_report_directory_set(const char* dir) {
    reportDirectory = dir ? dir : "";
}

// process.report.filename
static std::string reportFilename = "";

const char* nova_process_report_filename_get() {
    return reportFilename.c_str();
}

void nova_process_report_filename_set(const char* name) {
    reportFilename = name ? name : "";
}

// process.report.reportOnFatalError
static bool reportOnFatalError = false;

bool nova_process_report_reportOnFatalError_get() {
    return reportOnFatalError;
}

void nova_process_report_reportOnFatalError_set(bool value) {
    reportOnFatalError = value;
}

// process.report.reportOnSignal
static bool reportOnSignal = false;

bool nova_process_report_reportOnSignal_get() {
    return reportOnSignal;
}

void nova_process_report_reportOnSignal_set(bool value) {
    reportOnSignal = value;
}

// process.report.reportOnUncaughtException
static bool reportOnUncaughtException = false;

bool nova_process_report_reportOnUncaughtException_get() {
    return reportOnUncaughtException;
}

void nova_process_report_reportOnUncaughtException_set(bool value) {
    reportOnUncaughtException = value;
}

// process.report.signal
static std::string reportSignal = "SIGUSR2";

const char* nova_process_report_signal_get() {
    return reportSignal.c_str();
}

void nova_process_report_signal_set(const char* sig) {
    reportSignal = sig ? sig : "SIGUSR2";
}

// process.report.compact
static bool reportCompact = false;

bool nova_process_report_compact_get() {
    return reportCompact;
}

void nova_process_report_compact_set(bool value) {
    reportCompact = value;
}

// ============================================================================
// Active Resources
// ============================================================================

// process.getActiveResourcesInfo()
const char** nova_process_getActiveResourcesInfo(int* count) {
    static std::vector<const char*> resources;
    resources.clear();

    // Return basic resource types
    static const char* types[] = {"TCPSocketWrap", "TTYWrap", "FSReqCallback"};
    for (const char* t : types) {
        resources.push_back(t);
    }

    *count = static_cast<int>(resources.size());
    return resources.data();
}

// ============================================================================
// dlopen
// ============================================================================

// process.dlopen(module, filename, flags)
bool nova_process_dlopen(const char* filename, int flags) {
#ifdef _WIN32
    (void)flags;
    HMODULE handle = LoadLibraryA(filename);
    return handle != nullptr;
#else
    void* handle = dlopen(filename, flags ? flags : RTLD_LAZY);
    return handle != nullptr;
#endif
}

// ============================================================================
// IPC (for worker/cluster)
// ============================================================================

// process.send(message) - For IPC with parent
bool nova_process_send(const char* message) {
    if (!ipcConnected || ipcChannelFd < 0 || !message) {
        return false;
    }

    // Format message as JSON with newline delimiter (Node.js IPC protocol)
    std::string jsonMsg = "{\"type\":\"message\",\"data\":\"";
    jsonMsg += message;
    jsonMsg += "\"}\n";

#ifdef _WIN32
    // On Windows, write to the handle
    HANDLE hPipe = (HANDLE)(intptr_t)ipcChannelFd;
    DWORD written;
    if (WriteFile(hPipe, jsonMsg.c_str(), (DWORD)jsonMsg.length(), &written, NULL)) {
        return written == jsonMsg.length();
    }
    return false;
#else
    // On Unix, write to the file descriptor
    ssize_t written = write(ipcChannelFd, jsonMsg.c_str(), jsonMsg.length());
    return written == (ssize_t)jsonMsg.length();
#endif
}

// process.send with callback
bool nova_process_sendWithCallback(const char* message, void (*callback)(bool)) {
    bool result = nova_process_send(message);
    if (callback) {
        callback(result);
    }
    return result;
}

// process.disconnect() - Disconnect IPC
void nova_process_disconnect() {
    if (!ipcConnected) return;

#ifdef _WIN32
    if (ipcChannelFd >= 0) {
        CloseHandle((HANDLE)(intptr_t)ipcChannelFd);
    }
#else
    if (ipcChannelFd >= 0) {
        close(ipcChannelFd);
    }
#endif

    ipcChannelFd = -1;
    ipcConnected = false;
    ipcChannelPtr = nullptr;

    // Emit 'disconnect' event
    auto it = eventListeners.find("disconnect");
    if (it != eventListeners.end()) {
        for (auto& listener : it->second) {
            listener();
        }
    }
}

// ============================================================================
// Deprecation flags
// ============================================================================

static bool noDeprecation = false;
static bool throwDeprecation = false;
static bool traceDeprecation = false;

bool nova_process_noDeprecation_get() {
    return noDeprecation;
}

void nova_process_noDeprecation_set(bool value) {
    noDeprecation = value;
}

bool nova_process_throwDeprecation_get() {
    return throwDeprecation;
}

void nova_process_throwDeprecation_set(bool value) {
    throwDeprecation = value;
}

bool nova_process_traceDeprecation_get() {
    return traceDeprecation;
}

void nova_process_traceDeprecation_set(bool value) {
    traceDeprecation = value;
}

// ============================================================================
// Constraint functions
// ============================================================================

// process.constrainedMemory() - Returns constrained memory or 0
int64_t nova_process_constrainedMemory() {
    // Check for cgroups memory limit on Linux
#ifdef __linux__
    FILE* f = fopen("/sys/fs/cgroup/memory/memory.limit_in_bytes", "r");
    if (f) {
        int64_t limit;
        if (fscanf(f, "%ld", &limit) == 1) {
            fclose(f);
            return limit;
        }
        fclose(f);
    }
#endif
    return 0;
}

// process.availableMemory()
int64_t nova_process_availableMemory() {
#ifdef _WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        return status.ullAvailPhys;
    }
    return 0;
#else
    long pages = sysconf(_SC_AVPHYS_PAGES);
    long pageSize = sysconf(_SC_PAGESIZE);
    return pages * pageSize;
#endif
}

// ============================================================================
// Permission Model (Node.js 20+)
// ============================================================================

// process.permission.has(scope, reference)
bool nova_process_permission_has(const char* scope, const char* reference) {
    (void)scope;
    (void)reference;
    // Default: all permissions granted (no experimental permission model)
    return true;
}

// ============================================================================
// Cleanup
// ============================================================================

void nova_process_cleanup() {
    storedArgv.clear();
    storedExecArgv.clear();
    warningListeners.clear();
    eventListeners.clear();
    nextTickQueue.clear();
    uncaughtExceptionCallback = nullptr;
}

} // extern "C"
