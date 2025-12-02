// Nova Inspector Module - Node.js compatible V8 Inspector API
// Provides debugging and profiling capabilities

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <atomic>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

extern "C" {

// Forward declarations
void nova_inspector_waitForDebugger();

// Helper to allocate string
static char* allocString(const std::string& s) {
    char* result = (char*)malloc(s.size() + 1);
    if (result) {
        memcpy(result, s.c_str(), s.size() + 1);
    }
    return result;
}

// ============================================================================
// Inspector State
// ============================================================================

struct InspectorState {
    bool isOpen;
    int port;
    char* host;
    char* url;
    int socket;
    int clientSocket;  // Connected debugger client
    std::atomic<bool> waitingForDebugger;
    std::atomic<bool> debuggerConnected;
};

static InspectorState* globalInspector = nullptr;

static InspectorState* getInspector() {
    if (!globalInspector) {
        globalInspector = new InspectorState();
        globalInspector->isOpen = false;
        globalInspector->port = 9229;
        globalInspector->host = allocString("127.0.0.1");
        globalInspector->url = nullptr;
        globalInspector->socket = -1;
        globalInspector->clientSocket = -1;
        globalInspector->waitingForDebugger = false;
        globalInspector->debuggerConnected = false;
    }
    return globalInspector;
}

// ============================================================================
// Inspector Module Functions
// ============================================================================

// Open the inspector on a port
int nova_inspector_open(int port, const char* host, int wait) {
    InspectorState* state = getInspector();

    if (state->isOpen) {
        return 0; // Already open
    }

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    state->port = port > 0 ? port : 9229;

    free(state->host);
    state->host = allocString(host && *host ? host : "127.0.0.1");

    // Create WebSocket server for inspector protocol
    state->socket = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (state->socket < 0) {
        return -1;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(state->socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(state->socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(state->port);
    inet_pton(AF_INET, state->host, &addr.sin_addr);

    if (bind(state->socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
#ifdef _WIN32
        closesocket(state->socket);
#else
        close(state->socket);
#endif
        state->socket = -1;
        return -1;
    }

    if (listen(state->socket, 1) < 0) {
#ifdef _WIN32
        closesocket(state->socket);
#else
        close(state->socket);
#endif
        state->socket = -1;
        return -1;
    }

    state->isOpen = true;

    // Generate inspector URL
    char urlBuf[256];
    snprintf(urlBuf, sizeof(urlBuf), "ws://%s:%d/inspector", state->host, state->port);
    free(state->url);
    state->url = allocString(urlBuf);

    if (wait) {
        state->waitingForDebugger = true;
        // Wait for debugger connection
        nova_inspector_waitForDebugger();
    }

    return 0;
}

// Close the inspector
void nova_inspector_close() {
    InspectorState* state = getInspector();

    // Close client socket first
    if (state->clientSocket >= 0) {
#ifdef _WIN32
        closesocket(state->clientSocket);
#else
        close(state->clientSocket);
#endif
        state->clientSocket = -1;
    }

    // Close server socket
    if (state->socket >= 0) {
#ifdef _WIN32
        closesocket(state->socket);
#else
        close(state->socket);
#endif
        state->socket = -1;
    }

    state->isOpen = false;
    state->waitingForDebugger = false;
    state->debuggerConnected = false;

    free(state->url);
    state->url = nullptr;
}

// Get the inspector URL
char* nova_inspector_url() {
    InspectorState* state = getInspector();
    return state->url ? allocString(state->url) : nullptr;
}

// Wait for debugger to connect
void nova_inspector_waitForDebugger() {
    InspectorState* state = getInspector();

    if (!state->isOpen) {
        nova_inspector_open(9229, "127.0.0.1", 0);
    }

    state->waitingForDebugger = true;

    // Print inspector URL for users
    if (state->url) {
        fprintf(stderr, "Debugger listening on %s\n", state->url);
        fprintf(stderr, "For help, see: https://nodejs.org/en/docs/inspector\n");
    }

    // Wait for debugger connection using select/poll with timeout
    while (state->waitingForDebugger && !state->debuggerConnected) {
        if (state->socket < 0) break;

        // Use select to wait for incoming connection with timeout
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(state->socket, &readfds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;  // 100ms timeout

        int result = select(state->socket + 1, &readfds, nullptr, nullptr, &tv);

        if (result > 0 && FD_ISSET(state->socket, &readfds)) {
            // Accept incoming connection
            struct sockaddr_in clientAddr;
#ifdef _WIN32
            int addrLen = sizeof(clientAddr);
#else
            socklen_t addrLen = sizeof(clientAddr);
#endif
            int clientSocket = (int)accept(state->socket, (struct sockaddr*)&clientAddr, &addrLen);

            if (clientSocket >= 0) {
                state->clientSocket = clientSocket;
                state->debuggerConnected = true;
                state->waitingForDebugger = false;

                fprintf(stderr, "Debugger attached.\n");
                break;
            }
        }

        // Allow breaking out with Ctrl+C check (simplified)
#ifdef _WIN32
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            state->waitingForDebugger = false;
            break;
        }
#endif
    }
}

// Check if debugger is connected
int nova_inspector_isConnected() {
    InspectorState* state = getInspector();
    return state->debuggerConnected ? 1 : 0;
}

// Get client socket for communication
int nova_inspector_getClientSocket() {
    InspectorState* state = getInspector();
    return state->clientSocket;
}

// ============================================================================
// Inspector Session
// ============================================================================

struct InspectorSession {
    bool connected;
    bool connectedToMainThread;
    std::map<std::string, std::function<void(const char*)>> eventHandlers;
    int messageId;
    std::map<int, std::function<void(const char*, const char*)>> pendingCallbacks;
};

void* nova_inspector_Session_new() {
    InspectorSession* session = new InspectorSession();
    session->connected = false;
    session->connectedToMainThread = false;
    session->messageId = 0;
    return session;
}

void nova_inspector_Session_free(void* session) {
    InspectorSession* s = (InspectorSession*)session;
    if (s) {
        delete s;
    }
}

// Connect session to inspector back-end
int nova_inspector_Session_connect(void* session) {
    InspectorSession* s = (InspectorSession*)session;
    if (!s) return -1;

    if (s->connected) return 0;

    // In real implementation, would establish connection to V8 inspector
    s->connected = true;
    return 0;
}

// Connect to the main thread inspector
int nova_inspector_Session_connectToMainThread(void* session) {
    InspectorSession* s = (InspectorSession*)session;
    if (!s) return -1;

    if (s->connectedToMainThread) return 0;

    s->connected = true;
    s->connectedToMainThread = true;
    return 0;
}

// Disconnect from inspector
void nova_inspector_Session_disconnect(void* session) {
    InspectorSession* s = (InspectorSession*)session;
    if (s) {
        s->connected = false;
        s->connectedToMainThread = false;
    }
}

// Post a message to inspector
int nova_inspector_Session_post(void* session, const char* method, const char* params) {
    InspectorSession* s = (InspectorSession*)session;
    if (!s || !s->connected || !method) return -1;

    // Increment message ID
    int id = ++s->messageId;

    // In real implementation, would send Chrome DevTools Protocol message
    // Format: {"id": id, "method": method, "params": params}
    (void)params;

    return id;
}

// Post with callback
int nova_inspector_Session_postWithCallback(void* session, const char* method, const char* params, void* callback) {
    InspectorSession* s = (InspectorSession*)session;
    if (!s || !s->connected || !method) return -1;

    int id = nova_inspector_Session_post(session, method, params);

    if (callback && id > 0) {
        // Store callback for response
        // In real implementation, callback would be invoked when response arrives
    }

    return id;
}

// Register event handler
void nova_inspector_Session_on(void* session, const char* event, void* callback) {
    InspectorSession* s = (InspectorSession*)session;
    if (s && event) {
        // Store event handler
        // Events: 'inspectorNotification', specific protocol events
    }
    (void)callback;
}

// Remove event handler
void nova_inspector_Session_off(void* session, const char* event, void* callback) {
    InspectorSession* s = (InspectorSession*)session;
    if (s && event) {
        // Remove event handler
    }
    (void)callback;
}

// Add listener (alias for on)
void nova_inspector_Session_addListener(void* session, const char* event, void* callback) {
    nova_inspector_Session_on(session, event, callback);
}

// Remove listener (alias for off)
void nova_inspector_Session_removeListener(void* session, const char* event, void* callback) {
    nova_inspector_Session_off(session, event, callback);
}

// Once - register one-time event handler
void nova_inspector_Session_once(void* session, const char* event, void* callback) {
    InspectorSession* s = (InspectorSession*)session;
    if (s && event) {
        // Store one-time event handler
    }
    (void)callback;
}

// Remove all listeners
void nova_inspector_Session_removeAllListeners(void* session, const char* event) {
    InspectorSession* s = (InspectorSession*)session;
    if (s) {
        if (event) {
            s->eventHandlers.erase(event);
        } else {
            s->eventHandlers.clear();
        }
    }
}

// Get event names
char** nova_inspector_Session_eventNames(void* session, int* count) {
    InspectorSession* s = (InspectorSession*)session;
    if (!s || !count) return nullptr;

    *count = (int)s->eventHandlers.size();
    if (*count == 0) return nullptr;

    char** names = (char**)malloc(sizeof(char*) * (*count));
    int i = 0;
    for (const auto& pair : s->eventHandlers) {
        names[i++] = allocString(pair.first);
    }
    return names;
}

// Get listener count
int nova_inspector_Session_listenerCount(void* session, const char* event) {
    InspectorSession* s = (InspectorSession*)session;
    if (!s || !event) return 0;

    auto it = s->eventHandlers.find(event);
    return (it != s->eventHandlers.end()) ? 1 : 0;
}

// ============================================================================
// Inspector Console (for redirecting console output to inspector)
// ============================================================================

struct InspectorConsole {
    bool enabled;
};

static InspectorConsole* inspectorConsole = nullptr;

void* nova_inspector_console() {
    if (!inspectorConsole) {
        inspectorConsole = new InspectorConsole();
        inspectorConsole->enabled = false;
    }
    return inspectorConsole;
}

void nova_inspector_console_log(const char* message) {
    // Send console.log to inspector
    // In real implementation, would send Runtime.consoleAPICalled event
    (void)message;
}

void nova_inspector_console_warn(const char* message) {
    (void)message;
}

void nova_inspector_console_error(const char* message) {
    (void)message;
}

void nova_inspector_console_info(const char* message) {
    (void)message;
}

void nova_inspector_console_debug(const char* message) {
    (void)message;
}

void nova_inspector_console_dir(const char* object) {
    (void)object;
}

void nova_inspector_console_dirxml(const char* object) {
    (void)object;
}

void nova_inspector_console_table(const char* data) {
    (void)data;
}

void nova_inspector_console_trace(const char* message) {
    (void)message;
}

void nova_inspector_console_clear() {
    // Clear console in inspector
}

void nova_inspector_console_count(const char* label) {
    (void)label;
}

void nova_inspector_console_countReset(const char* label) {
    (void)label;
}

void nova_inspector_console_group(const char* label) {
    (void)label;
}

void nova_inspector_console_groupCollapsed(const char* label) {
    (void)label;
}

void nova_inspector_console_groupEnd() {
    // End console group
}

void nova_inspector_console_time(const char* label) {
    (void)label;
}

void nova_inspector_console_timeEnd(const char* label) {
    (void)label;
}

void nova_inspector_console_timeLog(const char* label) {
    (void)label;
}

void nova_inspector_console_timeStamp(const char* label) {
    (void)label;
}

void nova_inspector_console_profile(const char* label) {
    (void)label;
}

void nova_inspector_console_profileEnd(const char* label) {
    (void)label;
}

void nova_inspector_console_assert(int condition, const char* message) {
    (void)condition;
    (void)message;
}

// ============================================================================
// Network (inspector/promises)
// ============================================================================

// For async operations using promises
void* nova_inspector_Network_new() {
    return nullptr;
}

void nova_inspector_Network_free(void* network) {
    (void)network;
}

// ============================================================================
// Heap Profiler commands (via Session.post)
// ============================================================================

int nova_inspector_HeapProfiler_enable(void* session) {
    return nova_inspector_Session_post(session, "HeapProfiler.enable", nullptr);
}

int nova_inspector_HeapProfiler_disable(void* session) {
    return nova_inspector_Session_post(session, "HeapProfiler.disable", nullptr);
}

int nova_inspector_HeapProfiler_startTrackingHeapObjects(void* session, int trackAllocations) {
    char params[64];
    snprintf(params, sizeof(params), "{\"trackAllocations\":%s}", trackAllocations ? "true" : "false");
    return nova_inspector_Session_post(session, "HeapProfiler.startTrackingHeapObjects", params);
}

int nova_inspector_HeapProfiler_stopTrackingHeapObjects(void* session, int reportProgress) {
    char params[64];
    snprintf(params, sizeof(params), "{\"reportProgress\":%s}", reportProgress ? "true" : "false");
    return nova_inspector_Session_post(session, "HeapProfiler.stopTrackingHeapObjects", params);
}

int nova_inspector_HeapProfiler_takeHeapSnapshot(void* session, int reportProgress) {
    char params[64];
    snprintf(params, sizeof(params), "{\"reportProgress\":%s}", reportProgress ? "true" : "false");
    return nova_inspector_Session_post(session, "HeapProfiler.takeHeapSnapshot", params);
}

int nova_inspector_HeapProfiler_collectGarbage(void* session) {
    return nova_inspector_Session_post(session, "HeapProfiler.collectGarbage", nullptr);
}

int nova_inspector_HeapProfiler_getObjectByHeapObjectId(void* session, const char* objectId) {
    char params[128];
    snprintf(params, sizeof(params), "{\"objectId\":\"%s\"}", objectId ? objectId : "");
    return nova_inspector_Session_post(session, "HeapProfiler.getObjectByHeapObjectId", params);
}

int nova_inspector_HeapProfiler_getHeapObjectId(void* session, const char* objectId) {
    char params[128];
    snprintf(params, sizeof(params), "{\"objectId\":\"%s\"}", objectId ? objectId : "");
    return nova_inspector_Session_post(session, "HeapProfiler.getHeapObjectId", params);
}

int nova_inspector_HeapProfiler_startSampling(void* session, int samplingInterval) {
    char params[64];
    snprintf(params, sizeof(params), "{\"samplingInterval\":%d}", samplingInterval);
    return nova_inspector_Session_post(session, "HeapProfiler.startSampling", params);
}

int nova_inspector_HeapProfiler_stopSampling(void* session) {
    return nova_inspector_Session_post(session, "HeapProfiler.stopSampling", nullptr);
}

// ============================================================================
// Profiler commands (via Session.post)
// ============================================================================

int nova_inspector_Profiler_enable(void* session) {
    return nova_inspector_Session_post(session, "Profiler.enable", nullptr);
}

int nova_inspector_Profiler_disable(void* session) {
    return nova_inspector_Session_post(session, "Profiler.disable", nullptr);
}

int nova_inspector_Profiler_start(void* session) {
    return nova_inspector_Session_post(session, "Profiler.start", nullptr);
}

int nova_inspector_Profiler_stop(void* session) {
    return nova_inspector_Session_post(session, "Profiler.stop", nullptr);
}

int nova_inspector_Profiler_setSamplingInterval(void* session, int interval) {
    char params[64];
    snprintf(params, sizeof(params), "{\"interval\":%d}", interval);
    return nova_inspector_Session_post(session, "Profiler.setSamplingInterval", params);
}

int nova_inspector_Profiler_startPreciseCoverage(void* session, int callCount, int detailed) {
    char params[128];
    snprintf(params, sizeof(params), "{\"callCount\":%s,\"detailed\":%s}",
             callCount ? "true" : "false", detailed ? "true" : "false");
    return nova_inspector_Session_post(session, "Profiler.startPreciseCoverage", params);
}

int nova_inspector_Profiler_stopPreciseCoverage(void* session) {
    return nova_inspector_Session_post(session, "Profiler.stopPreciseCoverage", nullptr);
}

int nova_inspector_Profiler_takePreciseCoverage(void* session) {
    return nova_inspector_Session_post(session, "Profiler.takePreciseCoverage", nullptr);
}

int nova_inspector_Profiler_getBestEffortCoverage(void* session) {
    return nova_inspector_Session_post(session, "Profiler.getBestEffortCoverage", nullptr);
}

// ============================================================================
// Debugger commands (via Session.post)
// ============================================================================

int nova_inspector_Debugger_enable(void* session) {
    return nova_inspector_Session_post(session, "Debugger.enable", nullptr);
}

int nova_inspector_Debugger_disable(void* session) {
    return nova_inspector_Session_post(session, "Debugger.disable", nullptr);
}

int nova_inspector_Debugger_pause(void* session) {
    return nova_inspector_Session_post(session, "Debugger.pause", nullptr);
}

int nova_inspector_Debugger_resume(void* session) {
    return nova_inspector_Session_post(session, "Debugger.resume", nullptr);
}

int nova_inspector_Debugger_stepOver(void* session) {
    return nova_inspector_Session_post(session, "Debugger.stepOver", nullptr);
}

int nova_inspector_Debugger_stepInto(void* session) {
    return nova_inspector_Session_post(session, "Debugger.stepInto", nullptr);
}

int nova_inspector_Debugger_stepOut(void* session) {
    return nova_inspector_Session_post(session, "Debugger.stepOut", nullptr);
}

int nova_inspector_Debugger_setBreakpointByUrl(void* session, int lineNumber, const char* url, const char* condition) {
    char params[512];
    snprintf(params, sizeof(params), "{\"lineNumber\":%d,\"url\":\"%s\"%s%s%s}",
             lineNumber, url ? url : "",
             condition ? ",\"condition\":\"" : "",
             condition ? condition : "",
             condition ? "\"" : "");
    return nova_inspector_Session_post(session, "Debugger.setBreakpointByUrl", params);
}

int nova_inspector_Debugger_removeBreakpoint(void* session, const char* breakpointId) {
    char params[128];
    snprintf(params, sizeof(params), "{\"breakpointId\":\"%s\"}", breakpointId ? breakpointId : "");
    return nova_inspector_Session_post(session, "Debugger.removeBreakpoint", params);
}

int nova_inspector_Debugger_setBreakpointsActive(void* session, int active) {
    char params[32];
    snprintf(params, sizeof(params), "{\"active\":%s}", active ? "true" : "false");
    return nova_inspector_Session_post(session, "Debugger.setBreakpointsActive", params);
}

int nova_inspector_Debugger_setPauseOnExceptions(void* session, const char* state) {
    char params[64];
    snprintf(params, sizeof(params), "{\"state\":\"%s\"}", state ? state : "none");
    return nova_inspector_Session_post(session, "Debugger.setPauseOnExceptions", params);
}

int nova_inspector_Debugger_evaluateOnCallFrame(void* session, const char* callFrameId, const char* expression) {
    char params[1024];
    snprintf(params, sizeof(params), "{\"callFrameId\":\"%s\",\"expression\":\"%s\"}",
             callFrameId ? callFrameId : "", expression ? expression : "");
    return nova_inspector_Session_post(session, "Debugger.evaluateOnCallFrame", params);
}

int nova_inspector_Debugger_setVariableValue(void* session, int scopeNumber, const char* variableName, const char* newValue, const char* callFrameId) {
    char params[512];
    snprintf(params, sizeof(params), "{\"scopeNumber\":%d,\"variableName\":\"%s\",\"newValue\":%s,\"callFrameId\":\"%s\"}",
             scopeNumber, variableName ? variableName : "", newValue ? newValue : "null", callFrameId ? callFrameId : "");
    return nova_inspector_Session_post(session, "Debugger.setVariableValue", params);
}

int nova_inspector_Debugger_getScriptSource(void* session, const char* scriptId) {
    char params[128];
    snprintf(params, sizeof(params), "{\"scriptId\":\"%s\"}", scriptId ? scriptId : "");
    return nova_inspector_Session_post(session, "Debugger.getScriptSource", params);
}

int nova_inspector_Debugger_setScriptSource(void* session, const char* scriptId, const char* scriptSource) {
    // Note: scriptSource would need proper JSON escaping in real implementation
    char params[4096];
    snprintf(params, sizeof(params), "{\"scriptId\":\"%s\",\"scriptSource\":\"%s\"}",
             scriptId ? scriptId : "", scriptSource ? scriptSource : "");
    return nova_inspector_Session_post(session, "Debugger.setScriptSource", params);
}

// ============================================================================
// Runtime commands (via Session.post)
// ============================================================================

int nova_inspector_Runtime_enable(void* session) {
    return nova_inspector_Session_post(session, "Runtime.enable", nullptr);
}

int nova_inspector_Runtime_disable(void* session) {
    return nova_inspector_Session_post(session, "Runtime.disable", nullptr);
}

int nova_inspector_Runtime_evaluate(void* session, const char* expression) {
    char params[2048];
    snprintf(params, sizeof(params), "{\"expression\":\"%s\"}", expression ? expression : "");
    return nova_inspector_Session_post(session, "Runtime.evaluate", params);
}

int nova_inspector_Runtime_callFunctionOn(void* session, const char* functionDeclaration, const char* objectId) {
    char params[2048];
    snprintf(params, sizeof(params), "{\"functionDeclaration\":\"%s\",\"objectId\":\"%s\"}",
             functionDeclaration ? functionDeclaration : "", objectId ? objectId : "");
    return nova_inspector_Session_post(session, "Runtime.callFunctionOn", params);
}

int nova_inspector_Runtime_getProperties(void* session, const char* objectId, int ownProperties) {
    char params[256];
    snprintf(params, sizeof(params), "{\"objectId\":\"%s\",\"ownProperties\":%s}",
             objectId ? objectId : "", ownProperties ? "true" : "false");
    return nova_inspector_Session_post(session, "Runtime.getProperties", params);
}

int nova_inspector_Runtime_releaseObject(void* session, const char* objectId) {
    char params[128];
    snprintf(params, sizeof(params), "{\"objectId\":\"%s\"}", objectId ? objectId : "");
    return nova_inspector_Session_post(session, "Runtime.releaseObject", params);
}

int nova_inspector_Runtime_releaseObjectGroup(void* session, const char* objectGroup) {
    char params[128];
    snprintf(params, sizeof(params), "{\"objectGroup\":\"%s\"}", objectGroup ? objectGroup : "");
    return nova_inspector_Session_post(session, "Runtime.releaseObjectGroup", params);
}

int nova_inspector_Runtime_runIfWaitingForDebugger(void* session) {
    return nova_inspector_Session_post(session, "Runtime.runIfWaitingForDebugger", nullptr);
}

int nova_inspector_Runtime_getHeapUsage(void* session) {
    return nova_inspector_Session_post(session, "Runtime.getHeapUsage", nullptr);
}

int nova_inspector_Runtime_globalLexicalScopeNames(void* session) {
    return nova_inspector_Session_post(session, "Runtime.globalLexicalScopeNames", nullptr);
}

// ============================================================================
// Inspector Promises API (inspector/promises)
// Promise-based version of inspector.Session
// ============================================================================

struct InspectorPromiseSession {
    InspectorSession* session;
    bool connected;
};

void* nova_inspector_promises_Session_new() {
    InspectorPromiseSession* ps = new InspectorPromiseSession();
    ps->session = (InspectorSession*)nova_inspector_Session_new();
    ps->connected = false;
    return ps;
}

void nova_inspector_promises_Session_free(void* promiseSession) {
    InspectorPromiseSession* ps = (InspectorPromiseSession*)promiseSession;
    if (ps) {
        if (ps->session) {
            nova_inspector_Session_free(ps->session);
        }
        delete ps;
    }
}

// Connect returns a promise (represented as async operation)
int nova_inspector_promises_Session_connect(void* promiseSession) {
    InspectorPromiseSession* ps = (InspectorPromiseSession*)promiseSession;
    if (!ps || !ps->session) return -1;

    int result = nova_inspector_Session_connect(ps->session);
    ps->connected = (result == 0);
    return result;
}

// Connect to main thread returns a promise
int nova_inspector_promises_Session_connectToMainThread(void* promiseSession) {
    InspectorPromiseSession* ps = (InspectorPromiseSession*)promiseSession;
    if (!ps || !ps->session) return -1;

    int result = nova_inspector_Session_connectToMainThread(ps->session);
    ps->connected = (result == 0);
    return result;
}

// Disconnect (sync)
void nova_inspector_promises_Session_disconnect(void* promiseSession) {
    InspectorPromiseSession* ps = (InspectorPromiseSession*)promiseSession;
    if (ps && ps->session) {
        nova_inspector_Session_disconnect(ps->session);
        ps->connected = false;
    }
}

// Post returns a promise with the result
// Returns message ID, actual promise resolution handled by runtime
int nova_inspector_promises_Session_post(void* promiseSession, const char* method, const char* params) {
    InspectorPromiseSession* ps = (InspectorPromiseSession*)promiseSession;
    if (!ps || !ps->session) return -1;
    return nova_inspector_Session_post(ps->session, method, params);
}

// Event handlers (same as sync version)
void nova_inspector_promises_Session_on(void* promiseSession, const char* event, void* callback) {
    InspectorPromiseSession* ps = (InspectorPromiseSession*)promiseSession;
    if (ps && ps->session) {
        nova_inspector_Session_on(ps->session, event, callback);
    }
}

void nova_inspector_promises_Session_off(void* promiseSession, const char* event, void* callback) {
    InspectorPromiseSession* ps = (InspectorPromiseSession*)promiseSession;
    if (ps && ps->session) {
        nova_inspector_Session_off(ps->session, event, callback);
    }
}

void nova_inspector_promises_Session_once(void* promiseSession, const char* event, void* callback) {
    InspectorPromiseSession* ps = (InspectorPromiseSession*)promiseSession;
    if (ps && ps->session) {
        nova_inspector_Session_once(ps->session, event, callback);
    }
}

void nova_inspector_promises_Session_addListener(void* promiseSession, const char* event, void* callback) {
    nova_inspector_promises_Session_on(promiseSession, event, callback);
}

void nova_inspector_promises_Session_removeListener(void* promiseSession, const char* event, void* callback) {
    nova_inspector_promises_Session_off(promiseSession, event, callback);
}

void nova_inspector_promises_Session_removeAllListeners(void* promiseSession, const char* event) {
    InspectorPromiseSession* ps = (InspectorPromiseSession*)promiseSession;
    if (ps && ps->session) {
        nova_inspector_Session_removeAllListeners(ps->session, event);
    }
}

char** nova_inspector_promises_Session_eventNames(void* promiseSession, int* count) {
    InspectorPromiseSession* ps = (InspectorPromiseSession*)promiseSession;
    if (ps && ps->session) {
        return nova_inspector_Session_eventNames(ps->session, count);
    }
    if (count) *count = 0;
    return nullptr;
}

int nova_inspector_promises_Session_listenerCount(void* promiseSession, const char* event) {
    InspectorPromiseSession* ps = (InspectorPromiseSession*)promiseSession;
    if (ps && ps->session) {
        return nova_inspector_Session_listenerCount(ps->session, event);
    }
    return 0;
}

// ============================================================================
// Promises Debugger Domain
// ============================================================================

int nova_inspector_promises_Debugger_enable(void* promiseSession) {
    return nova_inspector_promises_Session_post(promiseSession, "Debugger.enable", nullptr);
}

int nova_inspector_promises_Debugger_disable(void* promiseSession) {
    return nova_inspector_promises_Session_post(promiseSession, "Debugger.disable", nullptr);
}

int nova_inspector_promises_Debugger_pause(void* promiseSession) {
    return nova_inspector_promises_Session_post(promiseSession, "Debugger.pause", nullptr);
}

int nova_inspector_promises_Debugger_resume(void* promiseSession) {
    return nova_inspector_promises_Session_post(promiseSession, "Debugger.resume", nullptr);
}

int nova_inspector_promises_Debugger_stepOver(void* promiseSession) {
    return nova_inspector_promises_Session_post(promiseSession, "Debugger.stepOver", nullptr);
}

int nova_inspector_promises_Debugger_stepInto(void* promiseSession) {
    return nova_inspector_promises_Session_post(promiseSession, "Debugger.stepInto", nullptr);
}

int nova_inspector_promises_Debugger_stepOut(void* promiseSession) {
    return nova_inspector_promises_Session_post(promiseSession, "Debugger.stepOut", nullptr);
}

int nova_inspector_promises_Debugger_setBreakpointByUrl(void* promiseSession, int lineNumber, const char* url, const char* condition) {
    char params[512];
    snprintf(params, sizeof(params), "{\"lineNumber\":%d,\"url\":\"%s\"%s%s%s}",
             lineNumber, url ? url : "",
             condition ? ",\"condition\":\"" : "",
             condition ? condition : "",
             condition ? "\"" : "");
    return nova_inspector_promises_Session_post(promiseSession, "Debugger.setBreakpointByUrl", params);
}

int nova_inspector_promises_Debugger_removeBreakpoint(void* promiseSession, const char* breakpointId) {
    char params[128];
    snprintf(params, sizeof(params), "{\"breakpointId\":\"%s\"}", breakpointId ? breakpointId : "");
    return nova_inspector_promises_Session_post(promiseSession, "Debugger.removeBreakpoint", params);
}

// ============================================================================
// Promises Profiler Domain
// ============================================================================

int nova_inspector_promises_Profiler_enable(void* promiseSession) {
    return nova_inspector_promises_Session_post(promiseSession, "Profiler.enable", nullptr);
}

int nova_inspector_promises_Profiler_disable(void* promiseSession) {
    return nova_inspector_promises_Session_post(promiseSession, "Profiler.disable", nullptr);
}

int nova_inspector_promises_Profiler_start(void* promiseSession) {
    return nova_inspector_promises_Session_post(promiseSession, "Profiler.start", nullptr);
}

int nova_inspector_promises_Profiler_stop(void* promiseSession) {
    return nova_inspector_promises_Session_post(promiseSession, "Profiler.stop", nullptr);
}

int nova_inspector_promises_Profiler_setSamplingInterval(void* promiseSession, int interval) {
    char params[64];
    snprintf(params, sizeof(params), "{\"interval\":%d}", interval);
    return nova_inspector_promises_Session_post(promiseSession, "Profiler.setSamplingInterval", params);
}

// ============================================================================
// Promises HeapProfiler Domain
// ============================================================================

int nova_inspector_promises_HeapProfiler_enable(void* promiseSession) {
    return nova_inspector_promises_Session_post(promiseSession, "HeapProfiler.enable", nullptr);
}

int nova_inspector_promises_HeapProfiler_disable(void* promiseSession) {
    return nova_inspector_promises_Session_post(promiseSession, "HeapProfiler.disable", nullptr);
}

int nova_inspector_promises_HeapProfiler_takeHeapSnapshot(void* promiseSession, int reportProgress) {
    char params[64];
    snprintf(params, sizeof(params), "{\"reportProgress\":%s}", reportProgress ? "true" : "false");
    return nova_inspector_promises_Session_post(promiseSession, "HeapProfiler.takeHeapSnapshot", params);
}

int nova_inspector_promises_HeapProfiler_collectGarbage(void* promiseSession) {
    return nova_inspector_promises_Session_post(promiseSession, "HeapProfiler.collectGarbage", nullptr);
}

int nova_inspector_promises_HeapProfiler_startSampling(void* promiseSession, int samplingInterval) {
    char params[64];
    snprintf(params, sizeof(params), "{\"samplingInterval\":%d}", samplingInterval);
    return nova_inspector_promises_Session_post(promiseSession, "HeapProfiler.startSampling", params);
}

int nova_inspector_promises_HeapProfiler_stopSampling(void* promiseSession) {
    return nova_inspector_promises_Session_post(promiseSession, "HeapProfiler.stopSampling", nullptr);
}

// ============================================================================
// Promises Runtime Domain
// ============================================================================

int nova_inspector_promises_Runtime_enable(void* promiseSession) {
    return nova_inspector_promises_Session_post(promiseSession, "Runtime.enable", nullptr);
}

int nova_inspector_promises_Runtime_disable(void* promiseSession) {
    return nova_inspector_promises_Session_post(promiseSession, "Runtime.disable", nullptr);
}

int nova_inspector_promises_Runtime_evaluate(void* promiseSession, const char* expression) {
    char params[2048];
    snprintf(params, sizeof(params), "{\"expression\":\"%s\"}", expression ? expression : "");
    return nova_inspector_promises_Session_post(promiseSession, "Runtime.evaluate", params);
}

int nova_inspector_promises_Runtime_getProperties(void* promiseSession, const char* objectId, int ownProperties) {
    char params[256];
    snprintf(params, sizeof(params), "{\"objectId\":\"%s\",\"ownProperties\":%s}",
             objectId ? objectId : "", ownProperties ? "true" : "false");
    return nova_inspector_promises_Session_post(promiseSession, "Runtime.getProperties", params);
}

int nova_inspector_promises_Runtime_getHeapUsage(void* promiseSession) {
    return nova_inspector_promises_Session_post(promiseSession, "Runtime.getHeapUsage", nullptr);
}

// ============================================================================
// Cleanup
// ============================================================================

void nova_inspector_cleanup() {
    if (globalInspector) {
        nova_inspector_close();
        free(globalInspector->host);
        free(globalInspector->url);
        delete globalInspector;
        globalInspector = nullptr;
    }
    if (inspectorConsole) {
        delete inspectorConsole;
        inspectorConsole = nullptr;
    }
#ifdef _WIN32
    WSACleanup();
#endif
}

} // extern "C"
