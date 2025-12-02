/**
 * nova:child_process - Child Process Module Implementation
 *
 * Provides child process spawning for Nova programs.
 * Compatible with Node.js child_process module.
 */

#include "nova/runtime/BuiltinModules.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <functional>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <spawn.h>
extern char **environ;
#endif

namespace nova {
namespace runtime {
namespace child_process {

// Helper to allocate and copy string
static char* allocString(const std::string& str) {
    char* result = (char*)malloc(str.length() + 1);
    if (result) {
        strcpy(result, str.c_str());
    }
    return result;
}

// ============================================================================
// EventEmitter Support
// ============================================================================

struct CPEventListener {
    void* callback;
    bool once;
};

// ============================================================================
// ChildProcess Structure
// ============================================================================

struct NovaChildProcess {
#ifdef _WIN32
    HANDLE hProcess;
    HANDLE hThread;
    DWORD pid;
    HANDLE hStdinWrite;
    HANDLE hStdoutRead;
    HANDLE hStderrRead;
#else
    pid_t pid;
    int stdinFd;
    int stdoutFd;
    int stderrFd;
#endif
    int exitCode;
    int signalCode;
    bool killed;
    bool connected;
    bool exited;
    char* spawnfile;
    char** spawnargs;
    int spawnargCount;

    // EventEmitter support
    std::map<std::string, std::vector<CPEventListener>> listeners;
};

extern "C" {

// ============================================================================
// Synchronous Functions
// ============================================================================

// execSync(command[, options]) - Execute command synchronously
char* nova_child_process_execSync(const char* command, int* exitCode) {
    if (!command) {
        if (exitCode) *exitCode = -1;
        return nullptr;
    }

    std::string output;

#ifdef _WIN32
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        if (exitCode) *exitCode = -1;
        return nullptr;
    }

    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi;
    char cmdLine[4096];
    snprintf(cmdLine, sizeof(cmdLine), "cmd.exe /c %s", command);

    if (!CreateProcessA(NULL, cmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        if (exitCode) *exitCode = -1;
        return nullptr;
    }

    CloseHandle(hWritePipe);

    char buffer[4096];
    DWORD bytesRead;
    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD code;
    GetExitCodeProcess(pi.hProcess, &code);
    if (exitCode) *exitCode = (int)code;

    CloseHandle(hReadPipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#else
    FILE* pipe = popen(command, "r");
    if (!pipe) {
        if (exitCode) *exitCode = -1;
        return nullptr;
    }

    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

    int status = pclose(pipe);
    if (exitCode) *exitCode = WEXITSTATUS(status);
#endif

    return allocString(output);
}

// execFileSync(file[, args][, options])
char* nova_child_process_execFileSync(const char* file, const char** args, int argCount, int* exitCode) {
    if (!file) {
        if (exitCode) *exitCode = -1;
        return nullptr;
    }

    std::string command = file;
    for (int i = 0; i < argCount; i++) {
        if (args[i]) {
            command += " ";
            command += args[i];
        }
    }

    return nova_child_process_execSync(command.c_str(), exitCode);
}

// spawnSync(command[, args][, options])
void* nova_child_process_spawnSync(const char* command, const char** args, int argCount) {
    NovaChildProcess* proc = (NovaChildProcess*)malloc(sizeof(NovaChildProcess));
    if (!proc) return nullptr;

    memset(proc, 0, sizeof(NovaChildProcess));
    proc->spawnfile = command ? allocString(command) : nullptr;

    std::string cmd = command ? command : "";
    for (int i = 0; i < argCount; i++) {
        if (args[i]) {
            cmd += " ";
            cmd += args[i];
        }
    }

    int code = 0;
    char* output = nova_child_process_execSync(cmd.c_str(), &code);
    if (output) free(output);

    proc->exitCode = code;
    proc->exited = true;
    proc->killed = false;
    proc->connected = false;

    return proc;
}

// ============================================================================
// Asynchronous Functions
// ============================================================================

// spawn(command[, args][, options])
void* nova_child_process_spawn(const char* command, const char** args, int argCount) {
    if (!command) return nullptr;

    NovaChildProcess* proc = (NovaChildProcess*)malloc(sizeof(NovaChildProcess));
    if (!proc) return nullptr;

    memset(proc, 0, sizeof(NovaChildProcess));
    proc->spawnfile = allocString(command);
    proc->exited = false;
    proc->killed = false;
    proc->connected = true;
    proc->exitCode = -1;
    proc->signalCode = 0;

#ifdef _WIN32
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    HANDLE hStdinRead, hStdinWrite;
    HANDLE hStdoutRead, hStdoutWrite;
    HANDLE hStderrRead, hStderrWrite;

    CreatePipe(&hStdinRead, &hStdinWrite, &sa, 0);
    CreatePipe(&hStdoutRead, &hStdoutWrite, &sa, 0);
    CreatePipe(&hStderrRead, &hStderrWrite, &sa, 0);

    SetHandleInformation(hStdinWrite, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hStdoutRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hStderrRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hStdinRead;
    si.hStdOutput = hStdoutWrite;
    si.hStdError = hStderrWrite;

    std::string cmdLine = command;
    for (int i = 0; i < argCount; i++) {
        if (args[i]) {
            cmdLine += " ";
            cmdLine += args[i];
        }
    }

    PROCESS_INFORMATION pi;
    if (CreateProcessA(NULL, (LPSTR)cmdLine.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        proc->hProcess = pi.hProcess;
        proc->hThread = pi.hThread;
        proc->pid = pi.dwProcessId;
        proc->hStdinWrite = hStdinWrite;
        proc->hStdoutRead = hStdoutRead;
        proc->hStderrRead = hStderrRead;

        CloseHandle(hStdinRead);
        CloseHandle(hStdoutWrite);
        CloseHandle(hStderrWrite);
    } else {
        CloseHandle(hStdinRead);
        CloseHandle(hStdinWrite);
        CloseHandle(hStdoutRead);
        CloseHandle(hStdoutWrite);
        CloseHandle(hStderrRead);
        CloseHandle(hStderrWrite);
        proc->pid = 0;
    }
#else
    int stdinPipe[2], stdoutPipe[2], stderrPipe[2];

    pipe(stdinPipe);
    pipe(stdoutPipe);
    pipe(stderrPipe);

    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        dup2(stdinPipe[0], STDIN_FILENO);
        dup2(stdoutPipe[1], STDOUT_FILENO);
        dup2(stderrPipe[1], STDERR_FILENO);

        close(stdinPipe[0]);
        close(stdinPipe[1]);
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        close(stderrPipe[0]);
        close(stderrPipe[1]);

        std::vector<char*> argv;
        argv.push_back((char*)command);
        for (int i = 0; i < argCount; i++) {
            argv.push_back((char*)args[i]);
        }
        argv.push_back(nullptr);

        execvp(command, argv.data());
        _exit(127);
    } else if (pid > 0) {
        proc->pid = pid;
        proc->stdinFd = stdinPipe[1];
        proc->stdoutFd = stdoutPipe[0];
        proc->stderrFd = stderrPipe[0];

        close(stdinPipe[0]);
        close(stdoutPipe[1]);
        close(stderrPipe[1]);
    } else {
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        close(stderrPipe[0]);
        close(stderrPipe[1]);
        proc->pid = 0;
    }
#endif

    return proc;
}

// exec(command[, options][, callback])
void* nova_child_process_exec(const char* command) {
    return nova_child_process_spawn(command, nullptr, 0);
}

// execFile(file[, args][, options][, callback])
void* nova_child_process_execFile(const char* file, const char** args, int argCount) {
    return nova_child_process_spawn(file, args, argCount);
}

// fork(modulePath[, args][, options]) - simplified, same as spawn
void* nova_child_process_fork(const char* modulePath, const char** args, int argCount) {
    return nova_child_process_spawn(modulePath, args, argCount);
}

// ============================================================================
// ChildProcess Instance Methods
// ============================================================================

// subprocess.pid
int nova_child_process_pid(void* proc) {
    if (!proc) return 0;
    NovaChildProcess* p = (NovaChildProcess*)proc;
#ifdef _WIN32
    return (int)p->pid;
#else
    return (int)p->pid;
#endif
}

// subprocess.kill([signal])
int nova_child_process_kill(void* proc, int signal) {
    if (!proc) return 0;
    NovaChildProcess* p = (NovaChildProcess*)proc;

    if (p->exited || p->killed) return 0;

#ifdef _WIN32
    (void)signal;
    if (TerminateProcess(p->hProcess, 1)) {
        p->killed = true;
        return 1;
    }
#else
    if (signal <= 0) signal = SIGTERM;
    if (::kill(p->pid, signal) == 0) {
        p->killed = true;
        p->signalCode = signal;
        return 1;
    }
#endif

    return 0;
}

// subprocess.killed
int nova_child_process_killed(void* proc) {
    return proc ? ((NovaChildProcess*)proc)->killed : 0;
}

// subprocess.exitCode
int nova_child_process_exitCode(void* proc) {
    if (!proc) return -1;
    NovaChildProcess* p = (NovaChildProcess*)proc;

    if (!p->exited) {
        // Check if process has exited
#ifdef _WIN32
        DWORD code;
        if (GetExitCodeProcess(p->hProcess, &code)) {
            if (code != STILL_ACTIVE) {
                p->exitCode = (int)code;
                p->exited = true;
            }
        }
#else
        int status;
        pid_t result = waitpid(p->pid, &status, WNOHANG);
        if (result > 0) {
            if (WIFEXITED(status)) {
                p->exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                p->signalCode = WTERMSIG(status);
            }
            p->exited = true;
        }
#endif
    }

    return p->exitCode;
}

// subprocess.signalCode
int nova_child_process_signalCode(void* proc) {
    return proc ? ((NovaChildProcess*)proc)->signalCode : 0;
}

// subprocess.connected
int nova_child_process_connected(void* proc) {
    return proc ? ((NovaChildProcess*)proc)->connected : 0;
}

// subprocess.disconnect()
void nova_child_process_disconnect(void* proc) {
    if (!proc) return;
    NovaChildProcess* p = (NovaChildProcess*)proc;
    p->connected = false;
}

// subprocess.ref()
void* nova_child_process_ref(void* proc) {
    return proc;
}

// subprocess.unref()
void* nova_child_process_unref(void* proc) {
    return proc;
}

// subprocess.spawnfile
char* nova_child_process_spawnfile(void* proc) {
    if (!proc) return nullptr;
    NovaChildProcess* p = (NovaChildProcess*)proc;
    return p->spawnfile ? allocString(p->spawnfile) : nullptr;
}

// Wait for process to exit
int nova_child_process_wait(void* proc) {
    if (!proc) return -1;
    NovaChildProcess* p = (NovaChildProcess*)proc;

    if (p->exited) return p->exitCode;

#ifdef _WIN32
    WaitForSingleObject(p->hProcess, INFINITE);
    DWORD code;
    GetExitCodeProcess(p->hProcess, &code);
    p->exitCode = (int)code;
#else
    int status;
    waitpid(p->pid, &status, 0);
    if (WIFEXITED(status)) {
        p->exitCode = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        p->signalCode = WTERMSIG(status);
    }
#endif

    p->exited = true;
    p->connected = false;
    return p->exitCode;
}

// ============================================================================
// IO Methods
// ============================================================================

// Write to stdin
int nova_child_process_stdin_write(void* proc, const char* data, int len) {
    if (!proc || !data) return 0;
    NovaChildProcess* p = (NovaChildProcess*)proc;

#ifdef _WIN32
    DWORD written;
    if (WriteFile(p->hStdinWrite, data, len, &written, NULL)) {
        return (int)written;
    }
#else
    return (int)write(p->stdinFd, data, len);
#endif

    return 0;
}

// Read from stdout
char* nova_child_process_stdout_read(void* proc) {
    if (!proc) return nullptr;
    NovaChildProcess* p = (NovaChildProcess*)proc;

    std::string output;
    char buffer[4096];

#ifdef _WIN32
    DWORD bytesRead;
    DWORD available;
    if (PeekNamedPipe(p->hStdoutRead, NULL, 0, NULL, &available, NULL) && available > 0) {
        while (ReadFile(p->hStdoutRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            output += buffer;
            if (!PeekNamedPipe(p->hStdoutRead, NULL, 0, NULL, &available, NULL) || available == 0) break;
        }
    }
#else
    ssize_t bytesRead;
    while ((bytesRead = read(p->stdoutFd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
    }
#endif

    return output.empty() ? nullptr : allocString(output);
}

// Read from stderr
char* nova_child_process_stderr_read(void* proc) {
    if (!proc) return nullptr;
    NovaChildProcess* p = (NovaChildProcess*)proc;

    std::string output;
    char buffer[4096];

#ifdef _WIN32
    DWORD bytesRead;
    DWORD available;
    if (PeekNamedPipe(p->hStderrRead, NULL, 0, NULL, &available, NULL) && available > 0) {
        while (ReadFile(p->hStderrRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            output += buffer;
            if (!PeekNamedPipe(p->hStderrRead, NULL, 0, NULL, &available, NULL) || available == 0) break;
        }
    }
#else
    ssize_t bytesRead;
    while ((bytesRead = read(p->stderrFd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
    }
#endif

    return output.empty() ? nullptr : allocString(output);
}

// Close stdin
void nova_child_process_stdin_end(void* proc) {
    if (!proc) return;
    NovaChildProcess* p = (NovaChildProcess*)proc;

#ifdef _WIN32
    if (p->hStdinWrite) {
        CloseHandle(p->hStdinWrite);
        p->hStdinWrite = NULL;
    }
#else
    if (p->stdinFd >= 0) {
        close(p->stdinFd);
        p->stdinFd = -1;
    }
#endif
}

// ============================================================================
// EventEmitter Methods
// ============================================================================

void* nova_child_process_on(void* proc, const char* event, void* listener) {
    if (!proc || !event || !listener) return proc;
    NovaChildProcess* p = (NovaChildProcess*)proc;

    CPEventListener l;
    l.callback = listener;
    l.once = false;
    p->listeners[event].push_back(l);

    return proc;
}

void* nova_child_process_once(void* proc, const char* event, void* listener) {
    if (!proc || !event || !listener) return proc;
    NovaChildProcess* p = (NovaChildProcess*)proc;

    CPEventListener l;
    l.callback = listener;
    l.once = true;
    p->listeners[event].push_back(l);

    return proc;
}

void* nova_child_process_off(void* proc, const char* event, void* listener) {
    if (!proc || !event) return proc;
    NovaChildProcess* p = (NovaChildProcess*)proc;

    auto it = p->listeners.find(event);
    if (it != p->listeners.end()) {
        if (listener) {
            // Remove specific listener
            auto& vec = it->second;
            vec.erase(std::remove_if(vec.begin(), vec.end(),
                [listener](const CPEventListener& l) { return l.callback == listener; }),
                vec.end());
        } else {
            // Remove all listeners for event
            p->listeners.erase(it);
        }
    }

    return proc;
}

void* nova_child_process_addListener(void* proc, const char* event, void* listener) {
    return nova_child_process_on(proc, event, listener);
}

void* nova_child_process_removeListener(void* proc, const char* event, void* listener) {
    return nova_child_process_off(proc, event, listener);
}

void* nova_child_process_removeAllListeners(void* proc, const char* event) {
    if (!proc) return proc;
    NovaChildProcess* p = (NovaChildProcess*)proc;

    if (event) {
        p->listeners.erase(event);
    } else {
        p->listeners.clear();
    }

    return proc;
}

int nova_child_process_emit(void* proc, const char* event) {
    if (!proc || !event) return 0;
    NovaChildProcess* p = (NovaChildProcess*)proc;

    auto it = p->listeners.find(event);
    if (it == p->listeners.end() || it->second.empty()) {
        return 0;  // No listeners
    }

    // Make copy to handle 'once' removal
    std::vector<CPEventListener> toCall = it->second;

    // Remove 'once' listeners before calling
    auto& vec = it->second;
    vec.erase(std::remove_if(vec.begin(), vec.end(),
        [](const CPEventListener& l) { return l.once; }),
        vec.end());

    // Call all listeners (callbacks would be invoked by generated code)
    return 1;
}

int nova_child_process_emitWithData(void* proc, const char* event, void* data) {
    if (!proc || !event) return 0;
    NovaChildProcess* p = (NovaChildProcess*)proc;

    auto it = p->listeners.find(event);
    if (it == p->listeners.end() || it->second.empty()) {
        return 0;  // No listeners
    }

    // Remove 'once' listeners
    auto& vec = it->second;
    vec.erase(std::remove_if(vec.begin(), vec.end(),
        [](const CPEventListener& l) { return l.once; }),
        vec.end());

    (void)data;  // Data passed by caller to callbacks
    return 1;
}

void** nova_child_process_listeners(void* proc, const char* event, int* count) {
    if (!proc || !event || !count) {
        if (count) *count = 0;
        return nullptr;
    }
    NovaChildProcess* p = (NovaChildProcess*)proc;

    auto it = p->listeners.find(event);
    if (it == p->listeners.end() || it->second.empty()) {
        *count = 0;
        return nullptr;
    }

    *count = (int)it->second.size();
    void** result = (void**)malloc(sizeof(void*) * (*count));
    for (int i = 0; i < *count; i++) {
        result[i] = it->second[i].callback;
    }

    return result;
}

int nova_child_process_listenerCount(void* proc, const char* event) {
    if (!proc || !event) return 0;
    NovaChildProcess* p = (NovaChildProcess*)proc;

    auto it = p->listeners.find(event);
    if (it == p->listeners.end()) return 0;
    return (int)it->second.size();
}

const char** nova_child_process_eventNames(void* proc, int* count) {
    if (!proc || !count) {
        if (count) *count = 0;
        return nullptr;
    }
    NovaChildProcess* p = (NovaChildProcess*)proc;

    *count = (int)p->listeners.size();
    if (*count == 0) return nullptr;

    const char** result = (const char**)malloc(sizeof(const char*) * (*count));
    int i = 0;
    for (const auto& pair : p->listeners) {
        result[i++] = allocString(pair.first);
    }

    return result;
}

void* nova_child_process_prependListener(void* proc, const char* event, void* listener) {
    if (!proc || !event || !listener) return proc;
    NovaChildProcess* p = (NovaChildProcess*)proc;

    CPEventListener l;
    l.callback = listener;
    l.once = false;
    p->listeners[event].insert(p->listeners[event].begin(), l);

    return proc;
}

void* nova_child_process_prependOnceListener(void* proc, const char* event, void* listener) {
    if (!proc || !event || !listener) return proc;
    NovaChildProcess* p = (NovaChildProcess*)proc;

    CPEventListener l;
    l.callback = listener;
    l.once = true;
    p->listeners[event].insert(p->listeners[event].begin(), l);

    return proc;
}

// ============================================================================
// Memory Management
// ============================================================================

void nova_child_process_free(void* proc) {
    if (!proc) return;
    NovaChildProcess* p = (NovaChildProcess*)proc;

#ifdef _WIN32
    if (p->hProcess) CloseHandle(p->hProcess);
    if (p->hThread) CloseHandle(p->hThread);
    if (p->hStdinWrite) CloseHandle(p->hStdinWrite);
    if (p->hStdoutRead) CloseHandle(p->hStdoutRead);
    if (p->hStderrRead) CloseHandle(p->hStderrRead);
#else
    if (p->stdinFd >= 0) close(p->stdinFd);
    if (p->stdoutFd >= 0) close(p->stdoutFd);
    if (p->stderrFd >= 0) close(p->stderrFd);
#endif

    if (p->spawnfile) free(p->spawnfile);
    if (p->spawnargs) {
        for (int i = 0; i < p->spawnargCount; i++) {
            if (p->spawnargs[i]) free(p->spawnargs[i]);
        }
        free(p->spawnargs);
    }

    free(p);
}

// ============================================================================
// Signal Constants
// ============================================================================

int nova_child_process_SIGTERM() { return 15; }
int nova_child_process_SIGKILL() { return 9; }
int nova_child_process_SIGINT() { return 2; }
int nova_child_process_SIGHUP() { return 1; }
int nova_child_process_SIGQUIT() { return 3; }

} // extern "C"

} // namespace child_process
} // namespace runtime
} // namespace nova
