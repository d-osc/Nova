/**
 * nova:os - OS Module Implementation
 *
 * Provides operating system utilities for Nova programs.
 */

#include "nova/runtime/BuiltinModules.h"
#include <cstring>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <cstdint>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <direct.h>
#define getcwd _getcwd
#define chdir _chdir
#else
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pwd.h>
#ifdef __linux__
#include <sys/sysinfo.h>
#endif
#ifdef __APPLE__
#include <sys/sysctl.h>
#endif
#endif

namespace nova {
namespace runtime {
namespace os {

// Helper to allocate and copy string
static char* allocString(const std::string& str) {
    char* result = (char*)malloc(str.length() + 1);
    if (result) {
        strcpy(result, str.c_str());
    }
    return result;
}

extern "C" {

// Get platform name
char* nova_os_platform() {
#ifdef _WIN32
    return allocString("win32");
#elif __APPLE__
    return allocString("darwin");
#elif __linux__
    return allocString("linux");
#elif __FreeBSD__
    return allocString("freebsd");
#else
    return allocString("unknown");
#endif
}

// Get architecture
char* nova_os_arch() {
#if defined(__x86_64__) || defined(_M_X64)
    return allocString("x64");
#elif defined(__i386__) || defined(_M_IX86)
    return allocString("x86");
#elif defined(__aarch64__) || defined(_M_ARM64)
    return allocString("arm64");
#elif defined(__arm__) || defined(_M_ARM)
    return allocString("arm");
#else
    return allocString("unknown");
#endif
}

// Get home directory
char* nova_os_homedir() {
#ifdef _WIN32
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile) {
        return allocString(userProfile);
    }
    const char* homeDrive = std::getenv("HOMEDRIVE");
    const char* homePath = std::getenv("HOMEPATH");
    if (homeDrive && homePath) {
        return allocString(std::string(homeDrive) + homePath);
    }
    return allocString("C:\\Users");
#else
    const char* home = std::getenv("HOME");
    if (home) {
        return allocString(home);
    }
    return allocString("/home");
#endif
}

// Get temp directory
char* nova_os_tmpdir() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    DWORD len = GetTempPathA(MAX_PATH, buffer);
    if (len > 0 && len < MAX_PATH) {
        // Remove trailing backslash
        if (buffer[len - 1] == '\\') {
            buffer[len - 1] = '\0';
        }
        return allocString(buffer);
    }
    return allocString("C:\\Windows\\Temp");
#else
    const char* tmpdir = std::getenv("TMPDIR");
    if (tmpdir) {
        return allocString(tmpdir);
    }
    return allocString("/tmp");
#endif
}

// Get hostname
char* nova_os_hostname() {
#ifdef _WIN32
    char buffer[256];
    DWORD size = sizeof(buffer);
    if (GetComputerNameA(buffer, &size)) {
        return allocString(buffer);
    }
    return allocString("localhost");
#else
    char buffer[256];
    if (gethostname(buffer, sizeof(buffer)) == 0) {
        return allocString(buffer);
    }
    return allocString("localhost");
#endif
}

// Get current working directory
char* nova_os_cwd() {
    char buffer[4096];
    if (getcwd(buffer, sizeof(buffer))) {
        return allocString(buffer);
    }
    return allocString(".");
}

// Change directory
int nova_os_chdir(const char* path) {
    if (!path) return 0;
    return chdir(path) == 0 ? 1 : 0;
}

// Get environment variable
char* nova_os_getenv(const char* name) {
    if (!name) return nullptr;
    const char* value = std::getenv(name);
    if (value) {
        return allocString(value);
    }
    return nullptr;
}

// Set environment variable
int nova_os_setenv(const char* name, const char* value) {
    if (!name) return 0;

#ifdef _WIN32
    if (value) {
        return _putenv_s(name, value) == 0 ? 1 : 0;
    } else {
        return _putenv_s(name, "") == 0 ? 1 : 0;
    }
#else
    if (value) {
        return setenv(name, value, 1) == 0 ? 1 : 0;
    } else {
        return unsetenv(name) == 0 ? 1 : 0;
    }
#endif
}

// Get CPU count
int nova_os_cpus() {
    return (int)std::thread::hardware_concurrency();
}

// Get total memory (bytes)
long long nova_os_totalmem() {
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        return (long long)memInfo.ullTotalPhys;
    }
    return 0;
#elif __APPLE__
    int64_t memsize;
    size_t len = sizeof(memsize);
    if (sysctlbyname("hw.memsize", &memsize, &len, NULL, 0) == 0) {
        return memsize;
    }
    return 0;
#elif __linux__
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return (long long)info.totalram * info.mem_unit;
    }
    return 0;
#else
    return 0;
#endif
}

// Get free memory (bytes)
long long nova_os_freemem() {
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        return (long long)memInfo.ullAvailPhys;
    }
    return 0;
#elif __linux__
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return (long long)info.freeram * info.mem_unit;
    }
    return 0;
#else
    return 0;
#endif
}

// Get uptime (seconds)
double nova_os_uptime() {
#ifdef _WIN32
    return (double)GetTickCount64() / 1000.0;
#elif __APPLE__
    struct timeval boottime;
    size_t len = sizeof(boottime);
    if (sysctlbyname("kern.boottime", &boottime, &len, NULL, 0) == 0) {
        time_t now = time(NULL);
        return (double)(now - boottime.tv_sec);
    }
    return 0;
#elif __linux__
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return (double)info.uptime;
    }
    return 0;
#else
    return 0;
#endif
}

// Exit process
void nova_os_exit(int code) {
    exit(code);
}

// os.EOL - line ending
char* nova_os_EOL() {
#ifdef _WIN32
    return allocString("\r\n");
#else
    return allocString("\n");
#endif
}

// os.devNull - null device path
char* nova_os_devNull() {
#ifdef _WIN32
    return allocString("\\\\.\\nul");
#else
    return allocString("/dev/null");
#endif
}

// os.availableParallelism() - number of parallelism available
int nova_os_availableParallelism() {
    return (int)std::thread::hardware_concurrency();
}

// os.endianness() - CPU endianness
char* nova_os_endianness() {
    uint16_t test = 0x0102;
    if (*((uint8_t*)&test) == 0x01) {
        return allocString("BE");
    }
    return allocString("LE");
}

// os.type() - OS type name
char* nova_os_type() {
#ifdef _WIN32
    return allocString("Windows_NT");
#elif __APPLE__
    return allocString("Darwin");
#elif __linux__
    return allocString("Linux");
#elif __FreeBSD__
    return allocString("FreeBSD");
#else
    return allocString("Unknown");
#endif
}

// os.release() - OS release version
char* nova_os_release() {
#ifdef _WIN32
    return allocString("10.0.0");
#else
    struct utsname info;
    if (uname(&info) == 0) {
        return allocString(info.release);
    }
    return allocString("unknown");
#endif
}

// os.version() - OS version string
char* nova_os_version() {
#ifdef _WIN32
    return allocString("Windows 10");
#else
    struct utsname info;
    if (uname(&info) == 0) {
        return allocString(info.version);
    }
    return allocString("unknown");
#endif
}

// os.machine() - machine type
char* nova_os_machine() {
#ifdef _WIN32
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    switch (sysInfo.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64: return allocString("x86_64");
        case PROCESSOR_ARCHITECTURE_ARM64: return allocString("aarch64");
        case PROCESSOR_ARCHITECTURE_INTEL: return allocString("i686");
        case PROCESSOR_ARCHITECTURE_ARM: return allocString("arm");
        default: return allocString("unknown");
    }
#else
    struct utsname info;
    if (uname(&info) == 0) {
        return allocString(info.machine);
    }
    return allocString("unknown");
#endif
}

// os.loadavg() - load averages (returns array as comma-separated string)
char* nova_os_loadavg() {
#ifdef _WIN32
    return allocString("0,0,0");
#else
    double loadavg[3] = {0, 0, 0};
    getloadavg(loadavg, 3);
    char buffer[64];
    sprintf(buffer, "%.2f,%.2f,%.2f", loadavg[0], loadavg[1], loadavg[2]);
    return allocString(buffer);
#endif
}

// os.getPriority([pid]) - get process priority
int nova_os_getPriority(int pid) {
#ifdef _WIN32
    HANDLE hProcess = (pid == 0) ? GetCurrentProcess() : OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (hProcess) {
        DWORD priority = GetPriorityClass(hProcess);
        if (pid != 0) CloseHandle(hProcess);
        switch (priority) {
            case REALTIME_PRIORITY_CLASS: return -20;
            case HIGH_PRIORITY_CLASS: return -14;
            case ABOVE_NORMAL_PRIORITY_CLASS: return -7;
            case NORMAL_PRIORITY_CLASS: return 0;
            case BELOW_NORMAL_PRIORITY_CLASS: return 7;
            case IDLE_PRIORITY_CLASS: return 19;
            default: return 0;
        }
    }
    return 0;
#else
    return getpriority(PRIO_PROCESS, pid);
#endif
}

// os.setPriority([pid,] priority) - set process priority
int nova_os_setPriority(int pid, int priority) {
#ifdef _WIN32
    HANDLE hProcess = (pid == 0) ? GetCurrentProcess() : OpenProcess(PROCESS_SET_INFORMATION, FALSE, pid);
    if (hProcess) {
        DWORD prioClass;
        if (priority <= -14) prioClass = HIGH_PRIORITY_CLASS;
        else if (priority <= -7) prioClass = ABOVE_NORMAL_PRIORITY_CLASS;
        else if (priority <= 0) prioClass = NORMAL_PRIORITY_CLASS;
        else if (priority <= 7) prioClass = BELOW_NORMAL_PRIORITY_CLASS;
        else prioClass = IDLE_PRIORITY_CLASS;
        int result = SetPriorityClass(hProcess, prioClass) ? 1 : 0;
        if (pid != 0) CloseHandle(hProcess);
        return result;
    }
    return 0;
#else
    return setpriority(PRIO_PROCESS, pid, priority) == 0 ? 1 : 0;
#endif
}

// os.userInfo() - returns user info
char* nova_os_userInfo_username() {
#ifdef _WIN32
    char buffer[256];
    DWORD size = sizeof(buffer);
    if (GetUserNameA(buffer, &size)) {
        return allocString(buffer);
    }
    return allocString("unknown");
#else
    const char* user = getenv("USER");
    if (user) return allocString(user);
    return allocString("unknown");
#endif
}

char* nova_os_userInfo_homedir() {
    return nova_os_homedir();
}

char* nova_os_userInfo_shell() {
#ifdef _WIN32
    const char* shell = getenv("COMSPEC");
    if (shell) return allocString(shell);
    return allocString("C:\\Windows\\System32\\cmd.exe");
#else
    const char* shell = getenv("SHELL");
    if (shell) return allocString(shell);
    return allocString("/bin/sh");
#endif
}

int nova_os_userInfo_uid() {
#ifdef _WIN32
    return -1;
#else
    return (int)getuid();
#endif
}

int nova_os_userInfo_gid() {
#ifdef _WIN32
    return -1;
#else
    return (int)getgid();
#endif
}

// os.networkInterfaces() - simplified
int nova_os_networkInterfaces_count() {
    return 1;
}

// os.constants - signal constants
int nova_os_constants_SIGINT() { return 2; }
int nova_os_constants_SIGTERM() { return 15; }
int nova_os_constants_SIGKILL() { return 9; }
int nova_os_constants_SIGHUP() { return 1; }
int nova_os_constants_SIGQUIT() { return 3; }
int nova_os_constants_SIGABRT() { return 6; }
int nova_os_constants_SIGALRM() { return 14; }
int nova_os_constants_SIGPIPE() { return 13; }
int nova_os_constants_SIGUSR1() { return 10; }
int nova_os_constants_SIGUSR2() { return 12; }

// os.constants - error constants
int nova_os_constants_ENOENT() { return 2; }
int nova_os_constants_EACCES() { return 13; }
int nova_os_constants_EEXIST() { return 17; }
int nova_os_constants_ENOTDIR() { return 20; }
int nova_os_constants_EISDIR() { return 21; }
int nova_os_constants_EINVAL() { return 22; }
int nova_os_constants_EMFILE() { return 24; }
int nova_os_constants_ENOTEMPTY() { return 39; }

// os.constants - priority constants
int nova_os_constants_PRIORITY_LOW() { return 19; }
int nova_os_constants_PRIORITY_BELOW_NORMAL() { return 10; }
int nova_os_constants_PRIORITY_NORMAL() { return 0; }
int nova_os_constants_PRIORITY_ABOVE_NORMAL() { return -7; }
int nova_os_constants_PRIORITY_HIGH() { return -14; }
int nova_os_constants_PRIORITY_HIGHEST() { return -20; }

} // extern "C"

} // namespace os
} // namespace runtime
} // namespace nova
