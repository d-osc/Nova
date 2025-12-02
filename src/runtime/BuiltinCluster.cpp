/**
 * nova:cluster - Cluster Module Implementation
 *
 * Provides cluster support for Nova programs.
 * Compatible with Node.js cluster module.
 */

#include "nova/runtime/BuiltinModules.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <map>
#include <vector>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#endif

namespace nova {
namespace runtime {
namespace cluster {

// Helper to allocate and copy string
static char* allocString(const std::string& str) {
    char* result = (char*)malloc(str.length() + 1);
    if (result) {
        strcpy(result, str.c_str());
    }
    return result;
}

// ============================================================================
// Scheduling Policies
// ============================================================================

static const int SCHED_NONE = 1;
static const int SCHED_RR = 2;  // Round-robin (default on non-Windows)

// ============================================================================
// Cluster Settings
// ============================================================================

struct ClusterSettings {
    char* exec;
    char** args;
    int argsCount;
    char* cwd;
    int silent;
    int schedulingPolicy;
    int uid;
    int gid;
    int inspectPort;
    char* serialization;  // 'json' or 'advanced'
    int windowsHide;
};

static ClusterSettings settings = {
    nullptr,  // exec
    nullptr,  // args
    0,        // argsCount
    nullptr,  // cwd
    0,        // silent
    SCHED_RR, // schedulingPolicy (default: round-robin)
    -1,       // uid
    -1,       // gid
    0,        // inspectPort
    nullptr,  // serialization
    0         // windowsHide
};

// ============================================================================
// Worker Structure
// ============================================================================

struct Worker {
    int id;
    int pid;
    int exitCode;
    int signalCode;
    int connected;
    int isDead;
    int exitedAfterDisconnect;
#ifdef _WIN32
    HANDLE processHandle;
#endif
    // Event callbacks
    void (*onOnline)(int workerId);
    void (*onListening)(int workerId, const char* address, int port);
    void (*onDisconnect)(int workerId);
    void (*onExit)(int workerId, int code, int signal);
    void (*onMessage)(int workerId, const char* message);
    void (*onError)(int workerId, const char* error);
};

static std::map<int, Worker*> workers;
static int nextWorkerId = 1;
static int isPrimaryProcess = 1;  // Assume primary until proven otherwise
static int currentWorkerId = 0;   // Set by worker process
static Worker* currentWorker = nullptr;

// ============================================================================
// Event Callbacks for Cluster
// ============================================================================

static void (*clusterOnFork)(int workerId) = nullptr;
static void (*clusterOnOnline)(int workerId) = nullptr;
static void (*clusterOnListening)(int workerId, const char* address, int port) = nullptr;
static void (*clusterOnDisconnect)(int workerId) = nullptr;
static void (*clusterOnExit)(int workerId, int code, int signal) = nullptr;
static void (*clusterOnMessage)(int workerId, const char* message) = nullptr;
static void (*clusterOnSetup)() = nullptr;

extern "C" {

// ============================================================================
// Constants
// ============================================================================

int nova_cluster_SCHED_NONE() {
    return SCHED_NONE;
}

int nova_cluster_SCHED_RR() {
    return SCHED_RR;
}

// ============================================================================
// Cluster State
// ============================================================================

// Check if this is the primary/master process
int nova_cluster_isPrimary() {
    return isPrimaryProcess;
}

// Alias for isPrimary (deprecated but still used)
int nova_cluster_isMaster() {
    return isPrimaryProcess;
}

// Check if this is a worker process
int nova_cluster_isWorker() {
    return !isPrimaryProcess;
}

// Get current worker (in worker process)
void* nova_cluster_worker() {
    return currentWorker;
}

// Get all workers (in primary process)
int nova_cluster_workersCount() {
    return (int)workers.size();
}

// Get worker by ID
void* nova_cluster_getWorker(int id) {
    auto it = workers.find(id);
    if (it != workers.end()) {
        return it->second;
    }
    return nullptr;
}

// Get all worker IDs
int* nova_cluster_getWorkerIds(int* count) {
    *count = (int)workers.size();
    if (*count == 0) return nullptr;

    int* ids = (int*)malloc(*count * sizeof(int));
    int i = 0;
    for (auto& pair : workers) {
        ids[i++] = pair.first;
    }
    return ids;
}

// ============================================================================
// Setup Functions
// ============================================================================

// Setup primary/master settings
void nova_cluster_setupPrimary(
    const char* exec,
    const char** args, int argsCount,
    const char* cwd,
    int silent,
    int schedulingPolicy
) {
    // Free previous settings
    if (settings.exec) free(settings.exec);
    if (settings.args) {
        for (int i = 0; i < settings.argsCount; i++) {
            free(settings.args[i]);
        }
        free(settings.args);
    }
    if (settings.cwd) free(settings.cwd);

    // Set new settings
    settings.exec = exec ? allocString(exec) : nullptr;
    settings.argsCount = argsCount;
    if (argsCount > 0 && args) {
        settings.args = (char**)malloc(argsCount * sizeof(char*));
        for (int i = 0; i < argsCount; i++) {
            settings.args[i] = args[i] ? allocString(args[i]) : nullptr;
        }
    } else {
        settings.args = nullptr;
    }
    settings.cwd = cwd ? allocString(cwd) : nullptr;
    settings.silent = silent;
    settings.schedulingPolicy = schedulingPolicy > 0 ? schedulingPolicy : SCHED_RR;

    // Trigger setup event
    if (clusterOnSetup) {
        clusterOnSetup();
    }
}

// Alias for setupPrimary (deprecated)
void nova_cluster_setupMaster(
    const char* exec,
    const char** args, int argsCount,
    const char* cwd,
    int silent,
    int schedulingPolicy
) {
    nova_cluster_setupPrimary(exec, args, argsCount, cwd, silent, schedulingPolicy);
}

// Get current settings
char* nova_cluster_settings_exec() {
    return settings.exec ? allocString(settings.exec) : nullptr;
}

char* nova_cluster_settings_cwd() {
    return settings.cwd ? allocString(settings.cwd) : nullptr;
}

int nova_cluster_settings_silent() {
    return settings.silent;
}

int nova_cluster_schedulingPolicy() {
    return settings.schedulingPolicy;
}

// ============================================================================
// Fork / Spawn Workers
// ============================================================================

// Fork a new worker process
void* nova_cluster_fork(const char** envVars, int envCount) {
    if (!isPrimaryProcess) {
        return nullptr;  // Only primary can fork
    }

    Worker* worker = new Worker();
    worker->id = nextWorkerId++;
    worker->exitCode = -1;
    worker->signalCode = 0;
    worker->connected = 1;
    worker->isDead = 0;
    worker->exitedAfterDisconnect = 0;
    worker->onOnline = nullptr;
    worker->onListening = nullptr;
    worker->onDisconnect = nullptr;
    worker->onExit = nullptr;
    worker->onMessage = nullptr;
    worker->onError = nullptr;

#ifdef _WIN32
    // Windows: use CreateProcess
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Build command line
    std::string cmdLine;
    if (settings.exec) {
        cmdLine = settings.exec;
    } else {
        // Use current executable
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        cmdLine = exePath;
    }

    // Add worker environment variable
    char envStr[256];
    snprintf(envStr, sizeof(envStr), "NOVA_WORKER_ID=%d", worker->id);
    _putenv(envStr);

    // Add custom environment variables
    for (int i = 0; i < envCount && envVars; i += 2) {
        if (envVars[i] && envVars[i+1]) {
            char envBuf[1024];
            snprintf(envBuf, sizeof(envBuf), "%s=%s", envVars[i], envVars[i+1]);
            _putenv(envBuf);
        }
    }

    if (CreateProcessA(
        NULL,
        (LPSTR)cmdLine.c_str(),
        NULL, NULL, FALSE,
        settings.windowsHide ? CREATE_NO_WINDOW : 0,
        NULL,
        settings.cwd,
        &si, &pi
    )) {
        worker->pid = pi.dwProcessId;
        worker->processHandle = pi.hProcess;
        CloseHandle(pi.hThread);
    } else {
        worker->pid = -1;
        worker->isDead = 1;
    }
#else
    // Unix: use fork()
    pid_t pid = fork();
    if (pid == 0) {
        // Child process (worker)
        isPrimaryProcess = 0;
        currentWorkerId = worker->id;
        currentWorker = worker;

        // Set environment
        char envStr[64];
        snprintf(envStr, sizeof(envStr), "%d", worker->id);
        setenv("NOVA_WORKER_ID", envStr, 1);

        // Add custom environment variables
        for (int i = 0; i < envCount && envVars; i += 2) {
            if (envVars[i] && envVars[i+1]) {
                setenv(envVars[i], envVars[i+1], 1);
            }
        }

        // If exec is set, replace process
        if (settings.exec) {
            if (settings.cwd) {
                chdir(settings.cwd);
            }
            execl(settings.exec, settings.exec, (char*)NULL);
            _exit(1);  // exec failed
        }

        return worker;
    } else if (pid > 0) {
        // Parent process
        worker->pid = pid;
    } else {
        // Fork failed
        worker->pid = -1;
        worker->isDead = 1;
    }
#endif

    workers[worker->id] = worker;

    // Trigger fork event
    if (clusterOnFork) {
        clusterOnFork(worker->id);
    }

    return worker;
}

// ============================================================================
// Worker Properties
// ============================================================================

int nova_cluster_Worker_id(void* workerPtr) {
    if (workerPtr) {
        return ((Worker*)workerPtr)->id;
    }
    return 0;
}

int nova_cluster_Worker_pid(void* workerPtr) {
    if (workerPtr) {
        return ((Worker*)workerPtr)->pid;
    }
    return 0;
}

int nova_cluster_Worker_exitCode(void* workerPtr) {
    if (workerPtr) {
        return ((Worker*)workerPtr)->exitCode;
    }
    return -1;
}

int nova_cluster_Worker_isDead(void* workerPtr) {
    if (workerPtr) {
        return ((Worker*)workerPtr)->isDead;
    }
    return 1;
}

int nova_cluster_Worker_isConnected(void* workerPtr) {
    if (workerPtr) {
        return ((Worker*)workerPtr)->connected;
    }
    return 0;
}

int nova_cluster_Worker_exitedAfterDisconnect(void* workerPtr) {
    if (workerPtr) {
        return ((Worker*)workerPtr)->exitedAfterDisconnect;
    }
    return 0;
}

// ============================================================================
// Worker Methods
// ============================================================================

// Send message to worker
int nova_cluster_Worker_send(void* workerPtr, const char* message) {
    if (!workerPtr || !message) return 0;

    Worker* worker = (Worker*)workerPtr;
    if (!worker->connected || worker->isDead) return 0;

    // In a real implementation, this would use IPC
    // For now, just trigger message event if callback is set
    if (worker->onMessage) {
        worker->onMessage(worker->id, message);
    }

    return 1;
}

// Kill worker
int nova_cluster_Worker_kill(void* workerPtr, int signal) {
    if (!workerPtr) return 0;

    Worker* worker = (Worker*)workerPtr;
    if (worker->isDead) return 0;

#ifdef _WIN32
    if (worker->processHandle) {
        TerminateProcess(worker->processHandle, signal);
        CloseHandle(worker->processHandle);
        worker->processHandle = NULL;
    }
#else
    if (worker->pid > 0) {
        kill(worker->pid, signal > 0 ? signal : SIGTERM);
    }
#endif

    worker->isDead = 1;
    worker->connected = 0;
    worker->signalCode = signal > 0 ? signal : 15;  // SIGTERM

    return 1;
}

// Disconnect worker
int nova_cluster_Worker_disconnect(void* workerPtr) {
    if (!workerPtr) return 0;

    Worker* worker = (Worker*)workerPtr;
    if (!worker->connected) return 0;

    worker->connected = 0;
    worker->exitedAfterDisconnect = 1;

    // Trigger disconnect event
    if (worker->onDisconnect) {
        worker->onDisconnect(worker->id);
    }
    if (clusterOnDisconnect) {
        clusterOnDisconnect(worker->id);
    }

    return 1;
}

// ============================================================================
// Worker Events
// ============================================================================

void nova_cluster_Worker_on(void* workerPtr, const char* event, void* callback) {
    if (!workerPtr || !event) return;

    Worker* worker = (Worker*)workerPtr;

    if (strcmp(event, "online") == 0) {
        worker->onOnline = (void (*)(int))callback;
    } else if (strcmp(event, "listening") == 0) {
        worker->onListening = (void (*)(int, const char*, int))callback;
    } else if (strcmp(event, "disconnect") == 0) {
        worker->onDisconnect = (void (*)(int))callback;
    } else if (strcmp(event, "exit") == 0) {
        worker->onExit = (void (*)(int, int, int))callback;
    } else if (strcmp(event, "message") == 0) {
        worker->onMessage = (void (*)(int, const char*))callback;
    } else if (strcmp(event, "error") == 0) {
        worker->onError = (void (*)(int, const char*))callback;
    }
}

void nova_cluster_Worker_once(void* workerPtr, const char* event, void* callback) {
    // Simplified: same as on()
    nova_cluster_Worker_on(workerPtr, event, callback);
}

void nova_cluster_Worker_off(void* workerPtr, const char* event) {
    if (!workerPtr || !event) return;

    Worker* worker = (Worker*)workerPtr;

    if (strcmp(event, "online") == 0) {
        worker->onOnline = nullptr;
    } else if (strcmp(event, "listening") == 0) {
        worker->onListening = nullptr;
    } else if (strcmp(event, "disconnect") == 0) {
        worker->onDisconnect = nullptr;
    } else if (strcmp(event, "exit") == 0) {
        worker->onExit = nullptr;
    } else if (strcmp(event, "message") == 0) {
        worker->onMessage = nullptr;
    } else if (strcmp(event, "error") == 0) {
        worker->onError = nullptr;
    }
}

// ============================================================================
// Cluster Events
// ============================================================================

void nova_cluster_on(const char* event, void* callback) {
    if (!event) return;

    if (strcmp(event, "fork") == 0) {
        clusterOnFork = (void (*)(int))callback;
    } else if (strcmp(event, "online") == 0) {
        clusterOnOnline = (void (*)(int))callback;
    } else if (strcmp(event, "listening") == 0) {
        clusterOnListening = (void (*)(int, const char*, int))callback;
    } else if (strcmp(event, "disconnect") == 0) {
        clusterOnDisconnect = (void (*)(int))callback;
    } else if (strcmp(event, "exit") == 0) {
        clusterOnExit = (void (*)(int, int, int))callback;
    } else if (strcmp(event, "message") == 0) {
        clusterOnMessage = (void (*)(int, const char*))callback;
    } else if (strcmp(event, "setup") == 0) {
        clusterOnSetup = (void (*)())callback;
    }
}

void nova_cluster_once(const char* event, void* callback) {
    nova_cluster_on(event, callback);
}

void nova_cluster_off(const char* event) {
    if (!event) return;

    if (strcmp(event, "fork") == 0) {
        clusterOnFork = nullptr;
    } else if (strcmp(event, "online") == 0) {
        clusterOnOnline = nullptr;
    } else if (strcmp(event, "listening") == 0) {
        clusterOnListening = nullptr;
    } else if (strcmp(event, "disconnect") == 0) {
        clusterOnDisconnect = nullptr;
    } else if (strcmp(event, "exit") == 0) {
        clusterOnExit = nullptr;
    } else if (strcmp(event, "message") == 0) {
        clusterOnMessage = nullptr;
    } else if (strcmp(event, "setup") == 0) {
        clusterOnSetup = nullptr;
    }
}

// ============================================================================
// Cluster Methods
// ============================================================================

// Disconnect all workers
void nova_cluster_disconnect(void (*callback)()) {
    for (auto& pair : workers) {
        Worker* worker = pair.second;
        if (worker->connected) {
            nova_cluster_Worker_disconnect(worker);
        }
    }

    if (callback) {
        callback();
    }
}

// ============================================================================
// Worker Lifecycle Triggers
// ============================================================================

// Called when worker comes online
void nova_cluster_triggerOnline(int workerId) {
    auto it = workers.find(workerId);
    if (it != workers.end()) {
        Worker* worker = it->second;
        if (worker->onOnline) {
            worker->onOnline(workerId);
        }
    }
    if (clusterOnOnline) {
        clusterOnOnline(workerId);
    }
}

// Called when worker starts listening
void nova_cluster_triggerListening(int workerId, const char* address, int port) {
    auto it = workers.find(workerId);
    if (it != workers.end()) {
        Worker* worker = it->second;
        if (worker->onListening) {
            worker->onListening(workerId, address, port);
        }
    }
    if (clusterOnListening) {
        clusterOnListening(workerId, address, port);
    }
}

// Called when worker exits
void nova_cluster_triggerExit(int workerId, int code, int signal) {
    auto it = workers.find(workerId);
    if (it != workers.end()) {
        Worker* worker = it->second;
        worker->exitCode = code;
        worker->signalCode = signal;
        worker->isDead = 1;
        worker->connected = 0;

        if (worker->onExit) {
            worker->onExit(workerId, code, signal);
        }
    }
    if (clusterOnExit) {
        clusterOnExit(workerId, code, signal);
    }
}

// Called when message received from worker
void nova_cluster_triggerMessage(int workerId, const char* message) {
    auto it = workers.find(workerId);
    if (it != workers.end()) {
        Worker* worker = it->second;
        if (worker->onMessage) {
            worker->onMessage(workerId, message);
        }
    }
    if (clusterOnMessage) {
        clusterOnMessage(workerId, message);
    }
}

// ============================================================================
// Utility Functions
// ============================================================================

// Check and reap dead workers
int nova_cluster_checkWorkers() {
    int deadCount = 0;

#ifndef _WIN32
    for (auto& pair : workers) {
        Worker* worker = pair.second;
        if (!worker->isDead && worker->pid > 0) {
            int status;
            pid_t result = waitpid(worker->pid, &status, WNOHANG);
            if (result > 0) {
                worker->isDead = 1;
                worker->connected = 0;
                if (WIFEXITED(status)) {
                    worker->exitCode = WEXITSTATUS(status);
                }
                if (WIFSIGNALED(status)) {
                    worker->signalCode = WTERMSIG(status);
                }
                deadCount++;

                // Trigger exit event
                nova_cluster_triggerExit(worker->id, worker->exitCode, worker->signalCode);
            }
        }
    }
#else
    for (auto& pair : workers) {
        Worker* worker = pair.second;
        if (!worker->isDead && worker->processHandle) {
            DWORD exitCode;
            if (GetExitCodeProcess(worker->processHandle, &exitCode)) {
                if (exitCode != STILL_ACTIVE) {
                    worker->isDead = 1;
                    worker->connected = 0;
                    worker->exitCode = (int)exitCode;
                    deadCount++;

                    CloseHandle(worker->processHandle);
                    worker->processHandle = NULL;

                    nova_cluster_triggerExit(worker->id, worker->exitCode, worker->signalCode);
                }
            }
        }
    }
#endif

    return deadCount;
}

// Initialize worker (called in worker process)
void nova_cluster_initWorker() {
    const char* workerIdStr = getenv("NOVA_WORKER_ID");
    if (workerIdStr) {
        isPrimaryProcess = 0;
        currentWorkerId = atoi(workerIdStr);

        // Create worker object for current process
        currentWorker = new Worker();
        currentWorker->id = currentWorkerId;
#ifdef _WIN32
        currentWorker->pid = _getpid();
#else
        currentWorker->pid = getpid();
#endif
        currentWorker->connected = 1;
        currentWorker->isDead = 0;
        currentWorker->exitCode = -1;
        currentWorker->signalCode = 0;
        currentWorker->exitedAfterDisconnect = 0;
    }
}

// Free worker
void nova_cluster_Worker_free(void* workerPtr) {
    if (workerPtr) {
        Worker* worker = (Worker*)workerPtr;
        workers.erase(worker->id);
#ifdef _WIN32
        if (worker->processHandle) {
            CloseHandle(worker->processHandle);
        }
#endif
        delete worker;
    }
}

// Cleanup all workers
void nova_cluster_cleanup() {
    for (auto& pair : workers) {
        delete pair.second;
    }
    workers.clear();

    if (settings.exec) free(settings.exec);
    if (settings.args) {
        for (int i = 0; i < settings.argsCount; i++) {
            free(settings.args[i]);
        }
        free(settings.args);
    }
    if (settings.cwd) free(settings.cwd);
    if (settings.serialization) free(settings.serialization);
}

} // extern "C"

} // namespace cluster
} // namespace runtime
} // namespace nova
