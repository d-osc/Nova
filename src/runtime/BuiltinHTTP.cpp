/**
 * nova:http - HTTP Module Implementation
 *
 * Provides HTTP server and client for Nova programs.
 * Compatible with Node.js http module.
 */

// Debug output control - set to 1 to enable debug output, 0 to disable
#define NOVA_HTTP_DEBUG 0

#if NOVA_HTTP_DEBUG
#define HTTP_DBG(...) fprintf(stderr, __VA_ARGS__)
#else
#define HTTP_DBG(...) do {} while(0)
#endif

#include "nova/runtime/BuiltinModules.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#include <map>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET socket_t;
#define INVALID_SOCK INVALID_SOCKET
#define CLOSE_SOCKET closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
typedef int socket_t;
#define INVALID_SOCK -1
#define CLOSE_SOCKET close
#endif

namespace nova {
namespace runtime {
namespace http {

// Helper to allocate and copy string
static char* allocString(const std::string& str) {
    char* result = (char*)malloc(str.length() + 1);
    if (result) {
        strcpy(result, str.c_str());
    }
    return result;
}

#ifdef _WIN32
static bool wsaInitialized = false;
static void initWinsock() {
    if (!wsaInitialized) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        wsaInitialized = true;
    }
}
#endif

// ============================================================================
// HTTP Status Codes
// ============================================================================

static std::map<int, std::string> statusCodes = {
    {100, "Continue"},
    {101, "Switching Protocols"},
    {102, "Processing"},
    {103, "Early Hints"},
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {203, "Non-Authoritative Information"},
    {204, "No Content"},
    {205, "Reset Content"},
    {206, "Partial Content"},
    {207, "Multi-Status"},
    {208, "Already Reported"},
    {226, "IM Used"},
    {300, "Multiple Choices"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {303, "See Other"},
    {304, "Not Modified"},
    {305, "Use Proxy"},
    {307, "Temporary Redirect"},
    {308, "Permanent Redirect"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {402, "Payment Required"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {406, "Not Acceptable"},
    {407, "Proxy Authentication Required"},
    {408, "Request Timeout"},
    {409, "Conflict"},
    {410, "Gone"},
    {411, "Length Required"},
    {412, "Precondition Failed"},
    {413, "Payload Too Large"},
    {414, "URI Too Long"},
    {415, "Unsupported Media Type"},
    {416, "Range Not Satisfiable"},
    {417, "Expectation Failed"},
    {418, "I'm a Teapot"},
    {421, "Misdirected Request"},
    {422, "Unprocessable Entity"},
    {423, "Locked"},
    {424, "Failed Dependency"},
    {425, "Too Early"},
    {426, "Upgrade Required"},
    {428, "Precondition Required"},
    {429, "Too Many Requests"},
    {431, "Request Header Fields Too Large"},
    {451, "Unavailable For Legal Reasons"},
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Timeout"},
    {505, "HTTP Version Not Supported"},
    {506, "Variant Also Negotiates"},
    {507, "Insufficient Storage"},
    {508, "Loop Detected"},
    {510, "Not Extended"},
    {511, "Network Authentication Required"}
};

// HTTP Methods
static const char* httpMethods[] = {
    "ACL", "BIND", "CHECKOUT", "CONNECT", "COPY", "DELETE",
    "GET", "HEAD", "LINK", "LOCK", "M-SEARCH", "MERGE",
    "MKACTIVITY", "MKCALENDAR", "MKCOL", "MOVE", "NOTIFY",
    "OPTIONS", "PATCH", "POST", "PRI", "PROPFIND", "PROPPATCH",
    "PURGE", "PUT", "REBIND", "REPORT", "SEARCH", "SOURCE",
    "SUBSCRIBE", "TRACE", "UNBIND", "UNLINK", "UNLOCK", "UNSUBSCRIBE"
};
static const int httpMethodsCount = sizeof(httpMethods) / sizeof(httpMethods[0]);

// ============================================================================
// Global Settings
// ============================================================================

static int maxHeaderSize = 16384;  // 16KB default
static int maxIdleHTTPParsers = 1000;

// ============================================================================
// IncomingMessage Structure (Request from client)
// ============================================================================

struct IncomingMessage {
    char* method;
    char* url;
    char* httpVersion;
    std::map<std::string, std::string> headers;
    char* body;
    int64_t bodyLength;
    int complete;
    socket_t socket;
    int statusCode;      // For responses
    char* statusMessage; // For responses
};

// ============================================================================
// ServerResponse Structure
// ============================================================================

struct ServerResponse {
    int statusCode;
    char* statusMessage;
    std::map<std::string, std::string> headers;
    int headersSent;
    int finished;
    socket_t socket;
    int chunkedEncoding;
    int keepAlive;
};

// ============================================================================
// ClientRequest Structure
// ============================================================================

struct ClientRequest {
    char* method;
    char* path;
    char* host;
    int port;
    std::map<std::string, std::string> headers;
    char* body;
    int64_t bodyLength;
    socket_t socket;
    int finished;
    int aborted;
    void (*onResponse)(void* req, void* res);
    void (*onError)(void* req, const char* error);
};

// ============================================================================
// Server Structure
// ============================================================================

struct Server {
    socket_t socket;
    int port;
    char* hostname;
    int listening;
    int maxConnections;
    int timeout;
    int keepAliveTimeout;
    int headersTimeout;
    int requestTimeout;
    void (*onRequest)(void* req, void* res);  // Node.js-compatible: (req, res) => void
    void (*onConnection)(void* server, void* socket);
    void (*onError)(void* server, const char* error);
    void (*onClose)(void* server);
    void (*onListening)(void* server);
};

// ============================================================================
// Agent Structure
// ============================================================================

struct Agent {
    int maxSockets;
    int maxFreeSockets;
    int maxTotalSockets;
    int timeout;
    int keepAlive;
    int keepAliveMsecs;
    int scheduling;  // 'fifo' or 'lifo'
    std::vector<socket_t> freeSockets;
    std::vector<socket_t> sockets;
};

static Agent* globalAgent = nullptr;

extern "C" {

// ============================================================================
// Module Constants
// ============================================================================

// Get HTTP methods array
char** nova_http_METHODS(int* count) {
    *count = httpMethodsCount;
    char** methods = (char**)malloc(httpMethodsCount * sizeof(char*));
    for (int i = 0; i < httpMethodsCount; i++) {
        methods[i] = allocString(httpMethods[i]);
    }
    return methods;
}

// Get status code text
char* nova_http_STATUS_CODES(int code) {
    auto it = statusCodes.find(code);
    if (it != statusCodes.end()) {
        return allocString(it->second);
    }
    return allocString("Unknown");
}

// Get all status codes
int* nova_http_STATUS_CODES_keys(int* count) {
    *count = (int)statusCodes.size();
    int* codes = (int*)malloc(*count * sizeof(int));
    int i = 0;
    for (auto& pair : statusCodes) {
        codes[i++] = pair.first;
    }
    return codes;
}

// Get max header size
int nova_http_maxHeaderSize() {
    return maxHeaderSize;
}

// Set max idle HTTP parsers
void nova_http_setMaxIdleHTTPParsers(int max) {
    if (max >= 0) {
        maxIdleHTTPParsers = max;
    }
}

// ============================================================================
// Header Validation
// ============================================================================

// Validate header name
int nova_http_validateHeaderName(const char* name) {
    if (!name || strlen(name) == 0) return 0;

    // Header name must be a valid token
    for (const char* p = name; *p; p++) {
        char c = *p;
        if (c <= 32 || c >= 127 || c == ':') {
            return 0;
        }
    }
    return 1;
}

// Validate header value
int nova_http_validateHeaderValue(const char* name, const char* value) {
    (void)name;
    if (!value) return 0;

    // Check for invalid characters
    for (const char* p = value; *p; p++) {
        char c = *p;
        if (c == '\r' || c == '\n') {
            return 0;
        }
    }
    return 1;
}

// ============================================================================
// Agent Functions
// ============================================================================

// Create new Agent
void* nova_http_Agent_new(int keepAlive, int keepAliveMsecs, int maxSockets, int maxFreeSockets, int timeout) {
    Agent* agent = new Agent();
    agent->keepAlive = keepAlive;
    agent->keepAliveMsecs = keepAliveMsecs > 0 ? keepAliveMsecs : 1000;
    agent->maxSockets = maxSockets > 0 ? maxSockets : 256;
    agent->maxFreeSockets = maxFreeSockets > 0 ? maxFreeSockets : 256;
    agent->maxTotalSockets = 0;  // Infinity
    agent->timeout = timeout;
    agent->scheduling = 0;  // LIFO
    return agent;
}

// Get global agent
void* nova_http_globalAgent() {
    if (!globalAgent) {
        globalAgent = (Agent*)nova_http_Agent_new(0, 1000, 256, 256, 0);
    }
    return globalAgent;
}

// Agent properties
int nova_http_Agent_maxSockets(void* agentPtr) {
    if (!agentPtr) return 256;
    return ((Agent*)agentPtr)->maxSockets;
}

int nova_http_Agent_maxFreeSockets(void* agentPtr) {
    if (!agentPtr) return 256;
    return ((Agent*)agentPtr)->maxFreeSockets;
}

int nova_http_Agent_keepAlive(void* agentPtr) {
    if (!agentPtr) return 0;
    return ((Agent*)agentPtr)->keepAlive;
}

// Destroy agent sockets
void nova_http_Agent_destroy(void* agentPtr) {
    if (!agentPtr) return;
    Agent* agent = (Agent*)agentPtr;

    for (auto& sock : agent->freeSockets) {
        CLOSE_SOCKET(sock);
    }
    agent->freeSockets.clear();

    for (auto& sock : agent->sockets) {
        CLOSE_SOCKET(sock);
    }
    agent->sockets.clear();
}

void nova_http_Agent_free(void* agentPtr) {
    if (!agentPtr) return;
    nova_http_Agent_destroy(agentPtr);
    delete (Agent*)agentPtr;
}

// ============================================================================
// Server Functions
// ============================================================================

// Create HTTP server
void* nova_http_createServer(void* requestListener) {
#ifdef _WIN32
    initWinsock();
#endif

    Server* server = new Server();
    server->socket = INVALID_SOCK;
    server->port = 0;
    server->hostname = nullptr;
    server->listening = 0;
    server->maxConnections = 0;
    server->timeout = 0;
    server->keepAliveTimeout = 5000;
    server->headersTimeout = 60000;
    server->requestTimeout = 300000;
    server->onRequest = (void (*)(void*, void*))requestListener;
    server->onConnection = nullptr;
    server->onError = nullptr;
    server->onClose = nullptr;
    server->onListening = nullptr;

    return server;
}

// Server.listen(port, hostname, callback)
int nova_http_Server_listen(void* serverPtr, int port, const char* hostname, void* callback) {
    HTTP_DBG("DEBUG nova_http_Server_listen: Called with port=%d\n", port);

    if (!serverPtr) {
        HTTP_DBG("DEBUG nova_http_Server_listen: serverPtr is NULL\n");
        return 0;
    }

    Server* server = (Server*)serverPtr;
    HTTP_DBG("DEBUG nova_http_Server_listen: server=%p\n", server);

#ifdef _WIN32
    initWinsock();
#endif

    server->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server->socket == INVALID_SOCK) {
#ifdef _WIN32
        int err = WSAGetLastError();
        (void)err;  // Suppress unused warning when debug disabled
        HTTP_DBG("DEBUG nova_http_Server_listen: socket() failed, WSAGetLastError()=%d\n", err);
#endif
        if (server->onError) {
            server->onError(serverPtr, "Failed to create socket");
        }
        return 0;
    }

    HTTP_DBG("DEBUG nova_http_Server_listen: socket created successfully, fd=%d\n", (int)server->socket);

    // Allow address reuse
    int opt = 1;
#ifdef _WIN32
    if (setsockopt(server->socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) != 0) {
        int err = WSAGetLastError();
        (void)err;  // Suppress unused warning when debug disabled
        HTTP_DBG("DEBUG nova_http_Server_listen: SO_REUSEADDR failed, WSAGetLastError()=%d\n", err);
    }
#else
    setsockopt(server->socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);

    if (hostname && strlen(hostname) > 0) {
        inet_pton(AF_INET, hostname, &addr.sin_addr);
        server->hostname = allocString(hostname);
    } else {
        addr.sin_addr.s_addr = INADDR_ANY;
        server->hostname = allocString("0.0.0.0");
    }

    HTTP_DBG("DEBUG nova_http_Server_listen: binding to %s:%d\n", server->hostname, port);

    if (bind(server->socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
#ifdef _WIN32
        int err = WSAGetLastError();
        (void)err;  // Suppress unused warning when debug disabled
        HTTP_DBG("DEBUG nova_http_Server_listen: bind() failed, WSAGetLastError()=%d\n", err);
#endif
        if (server->onError) {
            server->onError(serverPtr, "Failed to bind");
        }
        CLOSE_SOCKET(server->socket);
        server->socket = INVALID_SOCK;
        return 0;
    }

    HTTP_DBG("DEBUG nova_http_Server_listen: bind() successful\n");

    if (listen(server->socket, SOMAXCONN) < 0) {
#ifdef _WIN32
        int err = WSAGetLastError();
        (void)err;  // Suppress unused warning when debug disabled
        HTTP_DBG("DEBUG nova_http_Server_listen: listen() failed, WSAGetLastError()=%d\n", err);
#endif
        if (server->onError) {
            server->onError(serverPtr, "Failed to listen");
        }
        CLOSE_SOCKET(server->socket);
        server->socket = INVALID_SOCK;
        return 0;
    }

    HTTP_DBG("DEBUG nova_http_Server_listen: listen() successful, SOMAXCONN=%d\n", SOMAXCONN);

#ifdef _WIN32
    // Explicitly ensure socket is in blocking mode for select() to work properly
    u_long blockingMode = 0;  // 0 = blocking, 1 = non-blocking
    if (ioctlsocket(server->socket, FIONBIO, &blockingMode) != 0) {
        int err = WSAGetLastError();
        (void)err;  // Suppress unused warning when debug disabled
        HTTP_DBG("DEBUG nova_http_Server_listen: Warning - ioctlsocket(FIONBIO, 0) failed, WSAGetLastError()=%d\n", err);
    } else {
        HTTP_DBG("DEBUG nova_http_Server_listen: Socket explicitly set to blocking mode\n");
    }
#endif

    server->port = port;
    server->listening = 1;

    if (callback) {
        void (*cb)(void*) = (void (*)(void*))callback;
        cb(serverPtr);
    }

    if (server->onListening) {
        server->onListening(serverPtr);
    }

    return 1;
}

// Server.close()
void nova_http_Server_close(void* serverPtr, void* callback) {
    if (!serverPtr) return;

    Server* server = (Server*)serverPtr;

    if (server->socket != INVALID_SOCK) {
        CLOSE_SOCKET(server->socket);
        server->socket = INVALID_SOCK;
    }

    server->listening = 0;

    if (callback) {
        void (*cb)(void*) = (void (*)(void*))callback;
        cb(serverPtr);
    }

    if (server->onClose) {
        server->onClose(serverPtr);
    }
}

// Server properties
int nova_http_Server_listening(void* serverPtr) {
    if (!serverPtr) return 0;
    return ((Server*)serverPtr)->listening;
}

int nova_http_Server_maxConnections(void* serverPtr) {
    if (!serverPtr) return 0;
    return ((Server*)serverPtr)->maxConnections;
}

void nova_http_Server_setMaxConnections(void* serverPtr, int max) {
    if (serverPtr) {
        ((Server*)serverPtr)->maxConnections = max;
    }
}

int nova_http_Server_timeout(void* serverPtr) {
    if (!serverPtr) return 0;
    return ((Server*)serverPtr)->timeout;
}

void nova_http_Server_setTimeout(void* serverPtr, int ms, void* callback) {
    if (serverPtr) {
        ((Server*)serverPtr)->timeout = ms;
    }
    (void)callback;
}

int nova_http_Server_keepAliveTimeout(void* serverPtr) {
    if (!serverPtr) return 5000;
    return ((Server*)serverPtr)->keepAliveTimeout;
}

void nova_http_Server_setKeepAliveTimeout(void* serverPtr, int ms) {
    if (serverPtr) {
        ((Server*)serverPtr)->keepAliveTimeout = ms;
    }
}

int nova_http_Server_headersTimeout(void* serverPtr) {
    if (!serverPtr) return 60000;
    return ((Server*)serverPtr)->headersTimeout;
}

void nova_http_Server_setHeadersTimeout(void* serverPtr, int ms) {
    if (serverPtr) {
        ((Server*)serverPtr)->headersTimeout = ms;
    }
}

int nova_http_Server_requestTimeout(void* serverPtr) {
    if (!serverPtr) return 300000;
    return ((Server*)serverPtr)->requestTimeout;
}

void nova_http_Server_setRequestTimeout(void* serverPtr, int ms) {
    if (serverPtr) {
        ((Server*)serverPtr)->requestTimeout = ms;
    }
}

// Server event handlers
void nova_http_Server_on(void* serverPtr, const char* event, void* handler) {
    if (!serverPtr || !event) return;

    Server* server = (Server*)serverPtr;

    if (strcmp(event, "request") == 0) {
        server->onRequest = (void (*)(void*, void*))handler;
    } else if (strcmp(event, "connection") == 0) {
        server->onConnection = (void (*)(void*, void*))handler;
    } else if (strcmp(event, "error") == 0) {
        server->onError = (void (*)(void*, const char*))handler;
    } else if (strcmp(event, "close") == 0) {
        server->onClose = (void (*)(void*))handler;
    } else if (strcmp(event, "listening") == 0) {
        server->onListening = (void (*)(void*))handler;
    }
}

// Get server address
char* nova_http_Server_address_address(void* serverPtr) {
    if (!serverPtr) return nullptr;
    Server* server = (Server*)serverPtr;
    return server->hostname ? allocString(server->hostname) : allocString("0.0.0.0");
}

int nova_http_Server_address_port(void* serverPtr) {
    if (!serverPtr) return 0;
    return ((Server*)serverPtr)->port;
}

char* nova_http_Server_address_family(void* serverPtr) {
    (void)serverPtr;
    return allocString("IPv4");
}

void nova_http_Server_free(void* serverPtr) {
    if (!serverPtr) return;
    Server* server = (Server*)serverPtr;
    nova_http_Server_close(serverPtr, nullptr);
    if (server->hostname) free(server->hostname);
    delete server;
}

// ============================================================================
// IncomingMessage Functions
// ============================================================================

void* nova_http_IncomingMessage_new() {
    IncomingMessage* msg = new IncomingMessage();
    msg->method = nullptr;
    msg->url = nullptr;
    msg->httpVersion = allocString("1.1");
    msg->body = nullptr;
    msg->bodyLength = 0;
    msg->complete = 0;
    msg->socket = INVALID_SOCK;
    msg->statusCode = 0;
    msg->statusMessage = nullptr;
    return msg;
}

char* nova_http_IncomingMessage_method(void* msgPtr) {
    if (!msgPtr) return nullptr;
    IncomingMessage* msg = (IncomingMessage*)msgPtr;
    return msg->method ? allocString(msg->method) : nullptr;
}

char* nova_http_IncomingMessage_url(void* msgPtr) {
    if (!msgPtr) return nullptr;
    IncomingMessage* msg = (IncomingMessage*)msgPtr;
    return msg->url ? allocString(msg->url) : nullptr;
}

char* nova_http_IncomingMessage_httpVersion(void* msgPtr) {
    if (!msgPtr) return allocString("1.1");
    IncomingMessage* msg = (IncomingMessage*)msgPtr;
    return msg->httpVersion ? allocString(msg->httpVersion) : allocString("1.1");
}

int nova_http_IncomingMessage_statusCode(void* msgPtr) {
    if (!msgPtr) return 0;
    return ((IncomingMessage*)msgPtr)->statusCode;
}

char* nova_http_IncomingMessage_statusMessage(void* msgPtr) {
    if (!msgPtr) return nullptr;
    IncomingMessage* msg = (IncomingMessage*)msgPtr;
    return msg->statusMessage ? allocString(msg->statusMessage) : nullptr;
}

char* nova_http_IncomingMessage_getHeader(void* msgPtr, const char* name) {
    if (!msgPtr || !name) return nullptr;
    IncomingMessage* msg = (IncomingMessage*)msgPtr;

    std::string lowerName = name;
    for (auto& c : lowerName) c = (char)tolower(c);

    auto it = msg->headers.find(lowerName);
    if (it != msg->headers.end()) {
        return allocString(it->second);
    }
    return nullptr;
}

char** nova_http_IncomingMessage_headers(void* msgPtr, int* count) {
    if (!msgPtr) {
        *count = 0;
        return nullptr;
    }

    IncomingMessage* msg = (IncomingMessage*)msgPtr;
    *count = (int)msg->headers.size() * 2;
    if (*count == 0) return nullptr;

    char** headers = (char**)malloc(*count * sizeof(char*));
    int i = 0;
    for (auto& pair : msg->headers) {
        headers[i++] = allocString(pair.first);
        headers[i++] = allocString(pair.second);
    }
    return headers;
}

int nova_http_IncomingMessage_complete(void* msgPtr) {
    if (!msgPtr) return 0;
    return ((IncomingMessage*)msgPtr)->complete;
}

void nova_http_IncomingMessage_free(void* msgPtr) {
    if (!msgPtr) return;
    IncomingMessage* msg = (IncomingMessage*)msgPtr;
    if (msg->method) free(msg->method);
    if (msg->url) free(msg->url);
    if (msg->httpVersion) free(msg->httpVersion);
    if (msg->body) free(msg->body);
    if (msg->statusMessage) free(msg->statusMessage);
    delete msg;
}

// ============================================================================
// ServerResponse Functions
// ============================================================================

void* nova_http_ServerResponse_new(socket_t socket) {
    ServerResponse* res = new ServerResponse();
    res->statusCode = 200;
    res->statusMessage = allocString("OK");
    res->headersSent = 0;
    res->finished = 0;
    res->socket = socket;
    res->chunkedEncoding = 0;
    res->keepAlive = 1;
    return res;
}

void nova_http_ServerResponse_setStatusCode(void* resPtr, int code) {
    if (!resPtr) return;
    ServerResponse* res = (ServerResponse*)resPtr;
    res->statusCode = code;
    if (res->statusMessage) free(res->statusMessage);
    auto it = statusCodes.find(code);
    res->statusMessage = allocString(it != statusCodes.end() ? it->second : "Unknown");
}

int nova_http_ServerResponse_statusCode(void* resPtr) {
    if (!resPtr) return 200;
    return ((ServerResponse*)resPtr)->statusCode;
}

void nova_http_ServerResponse_setStatusMessage(void* resPtr, const char* message) {
    if (!resPtr) return;
    ServerResponse* res = (ServerResponse*)resPtr;
    if (res->statusMessage) free(res->statusMessage);
    res->statusMessage = message ? allocString(message) : nullptr;
}

char* nova_http_ServerResponse_statusMessage(void* resPtr) {
    if (!resPtr) return allocString("OK");
    ServerResponse* res = (ServerResponse*)resPtr;
    return res->statusMessage ? allocString(res->statusMessage) : allocString("OK");
}

void nova_http_ServerResponse_setHeader(void* resPtr, const char* name, const char* value) {
    if (!resPtr || !name || !value) return;
    ServerResponse* res = (ServerResponse*)resPtr;
    if (res->headersSent) return;

    std::string lowerName = name;
    for (auto& c : lowerName) c = (char)tolower(c);
    res->headers[lowerName] = value;
}

char* nova_http_ServerResponse_getHeader(void* resPtr, const char* name) {
    if (!resPtr || !name) return nullptr;
    ServerResponse* res = (ServerResponse*)resPtr;

    std::string lowerName = name;
    for (auto& c : lowerName) c = (char)tolower(c);

    auto it = res->headers.find(lowerName);
    if (it != res->headers.end()) {
        return allocString(it->second);
    }
    return nullptr;
}

void nova_http_ServerResponse_removeHeader(void* resPtr, const char* name) {
    if (!resPtr || !name) return;
    ServerResponse* res = (ServerResponse*)resPtr;
    if (res->headersSent) return;

    std::string lowerName = name;
    for (auto& c : lowerName) c = (char)tolower(c);
    res->headers.erase(lowerName);
}

int nova_http_ServerResponse_hasHeader(void* resPtr, const char* name) {
    if (!resPtr || !name) return 0;
    ServerResponse* res = (ServerResponse*)resPtr;

    std::string lowerName = name;
    for (auto& c : lowerName) c = (char)tolower(c);
    return res->headers.find(lowerName) != res->headers.end() ? 1 : 0;
}

char** nova_http_ServerResponse_getHeaderNames(void* resPtr, int* count) {
    if (!resPtr) {
        *count = 0;
        return nullptr;
    }

    ServerResponse* res = (ServerResponse*)resPtr;
    *count = (int)res->headers.size();
    if (*count == 0) return nullptr;

    char** names = (char**)malloc(*count * sizeof(char*));
    int i = 0;
    for (auto& pair : res->headers) {
        names[i++] = allocString(pair.first);
    }
    return names;
}

int nova_http_ServerResponse_headersSent(void* resPtr) {
    if (!resPtr) return 0;
    return ((ServerResponse*)resPtr)->headersSent;
}

void nova_http_ServerResponse_writeHead(void* resPtr, int statusCode, const char* statusMessage) {
    if (!resPtr) return;
    ServerResponse* res = (ServerResponse*)resPtr;
    if (res->headersSent) return;

    res->statusCode = statusCode;
    if (res->statusMessage) free(res->statusMessage);
    if (statusMessage) {
        res->statusMessage = allocString(statusMessage);
    } else {
        auto it = statusCodes.find(statusCode);
        res->statusMessage = allocString(it != statusCodes.end() ? it->second : "Unknown");
    }
}

int nova_http_ServerResponse_write(void* resPtr, const char* data, int length) {
    if (!resPtr || !data) return 0;
    ServerResponse* res = (ServerResponse*)resPtr;
    if (res->finished) return 0;

    // Send headers if not sent
    if (!res->headersSent) {
        std::string headerStr = "HTTP/1.1 " + std::to_string(res->statusCode) + " " +
            (res->statusMessage ? res->statusMessage : "OK") + "\r\n";

        for (auto& pair : res->headers) {
            headerStr += pair.first + ": " + pair.second + "\r\n";
        }
        headerStr += "\r\n";

        send(res->socket, headerStr.c_str(), (int)headerStr.length(), 0);
        res->headersSent = 1;
    }

    // Send data
    int len = length > 0 ? length : (int)strlen(data);
    return send(res->socket, data, len, 0) > 0 ? 1 : 0;
}

void nova_http_ServerResponse_end(void* resPtr, const char* data, int length) {
    HTTP_DBG("DEBUG nova_http_ServerResponse_end: Called with resPtr=%p, data=%p, length=%d\n",
            resPtr, (void*)data, length);

    if (!resPtr) {
        HTTP_DBG("DEBUG nova_http_ServerResponse_end: resPtr is NULL\n");
        return;
    }

    ServerResponse* res = (ServerResponse*)resPtr;
    if (res->finished) {
        HTTP_DBG("DEBUG nova_http_ServerResponse_end: Already finished\n");
        return;
    }

    if (data) {
        HTTP_DBG("DEBUG nova_http_ServerResponse_end: Sending data: '%s'\n", data);
        nova_http_ServerResponse_write(resPtr, data, length);
    } else if (!res->headersSent) {
        HTTP_DBG("DEBUG nova_http_ServerResponse_end: Sending empty response\n");
        nova_http_ServerResponse_write(resPtr, "", 0);
    }

    res->finished = 1;
    HTTP_DBG("DEBUG nova_http_ServerResponse_end: Response marked as finished\n");
}

int nova_http_ServerResponse_finished(void* resPtr) {
    if (!resPtr) return 1;
    return ((ServerResponse*)resPtr)->finished;
}

void nova_http_ServerResponse_free(void* resPtr) {
    if (!resPtr) return;
    ServerResponse* res = (ServerResponse*)resPtr;
    if (res->statusMessage) free(res->statusMessage);
    delete res;
}

// ============================================================================
// ClientRequest Functions
// ============================================================================

void* nova_http_request(const char* url, const char* method, void* callback) {
#ifdef _WIN32
    initWinsock();
#endif

    ClientRequest* req = new ClientRequest();
    req->method = method ? allocString(method) : allocString("GET");
    req->path = allocString("/");
    req->host = nullptr;
    req->port = 80;
    req->body = nullptr;
    req->bodyLength = 0;
    req->socket = INVALID_SOCK;
    req->finished = 0;
    req->aborted = 0;
    req->onResponse = (void (*)(void*, void*))callback;
    req->onError = nullptr;

    // Parse URL (simplified)
    if (url) {
        std::string urlStr = url;
        size_t hostStart = 0;
        if (urlStr.substr(0, 7) == "http://") {
            hostStart = 7;
        }

        size_t pathStart = urlStr.find('/', hostStart);
        if (pathStart != std::string::npos) {
            req->host = allocString(urlStr.substr(hostStart, pathStart - hostStart));
            req->path = allocString(urlStr.substr(pathStart));
        } else {
            req->host = allocString(urlStr.substr(hostStart));
        }

        // Check for port
        if (req->host) {
            std::string hostStr = req->host;
            size_t colonPos = hostStr.find(':');
            if (colonPos != std::string::npos) {
                req->port = atoi(hostStr.substr(colonPos + 1).c_str());
                free(req->host);
                req->host = allocString(hostStr.substr(0, colonPos));
            }
        }
    }

    return req;
}

void* nova_http_get(const char* url, void* callback) {
    return nova_http_request(url, "GET", callback);
}

void nova_http_ClientRequest_setHeader(void* reqPtr, const char* name, const char* value) {
    if (!reqPtr || !name || !value) return;
    ClientRequest* req = (ClientRequest*)reqPtr;
    req->headers[name] = value;
}

int nova_http_ClientRequest_write(void* reqPtr, const char* data, int length) {
    if (!reqPtr || !data) return 0;
    ClientRequest* req = (ClientRequest*)reqPtr;

    int len = length > 0 ? length : (int)strlen(data);
    req->body = (char*)realloc(req->body, req->bodyLength + len + 1);
    memcpy(req->body + req->bodyLength, data, len);
    req->bodyLength += len;
    req->body[req->bodyLength] = '\0';

    return 1;
}

void nova_http_ClientRequest_end(void* reqPtr, const char* data, int length) {
    if (!reqPtr) return;
    ClientRequest* req = (ClientRequest*)reqPtr;
    if (req->finished || req->aborted) return;

    if (data) {
        nova_http_ClientRequest_write(reqPtr, data, length);
    }

    // Connect and send request
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char portStr[16];
    snprintf(portStr, sizeof(portStr), "%d", req->port);

    if (getaddrinfo(req->host, portStr, &hints, &res) != 0) {
        if (req->onError) {
            req->onError(reqPtr, "Failed to resolve host");
        }
        return;
    }

    req->socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (req->socket == INVALID_SOCK) {
        freeaddrinfo(res);
        if (req->onError) {
            req->onError(reqPtr, "Failed to create socket");
        }
        return;
    }

    if (connect(req->socket, res->ai_addr, (int)res->ai_addrlen) < 0) {
        CLOSE_SOCKET(req->socket);
        freeaddrinfo(res);
        if (req->onError) {
            req->onError(reqPtr, "Failed to connect");
        }
        return;
    }

    freeaddrinfo(res);

    // Build HTTP request
    std::string httpReq = std::string(req->method) + " " + req->path + " HTTP/1.1\r\n";
    httpReq += "Host: " + std::string(req->host) + "\r\n";

    for (auto& pair : req->headers) {
        httpReq += pair.first + ": " + pair.second + "\r\n";
    }

    if (req->bodyLength > 0) {
        httpReq += "Content-Length: " + std::to_string(req->bodyLength) + "\r\n";
    }

    httpReq += "\r\n";

    if (req->body) {
        httpReq += req->body;
    }

    send(req->socket, httpReq.c_str(), (int)httpReq.length(), 0);
    req->finished = 1;

    // In a real implementation, we would read response asynchronously
    // For now, this is simplified
}

void nova_http_ClientRequest_abort(void* reqPtr) {
    if (!reqPtr) return;
    ClientRequest* req = (ClientRequest*)reqPtr;
    req->aborted = 1;
    if (req->socket != INVALID_SOCK) {
        CLOSE_SOCKET(req->socket);
        req->socket = INVALID_SOCK;
    }
}

int nova_http_ClientRequest_aborted(void* reqPtr) {
    if (!reqPtr) return 0;
    return ((ClientRequest*)reqPtr)->aborted;
}

void nova_http_ClientRequest_on(void* reqPtr, const char* event, void* handler) {
    if (!reqPtr || !event) return;
    ClientRequest* req = (ClientRequest*)reqPtr;

    if (strcmp(event, "response") == 0) {
        req->onResponse = (void (*)(void*, void*))handler;
    } else if (strcmp(event, "error") == 0) {
        req->onError = (void (*)(void*, const char*))handler;
    }
}

void nova_http_ClientRequest_free(void* reqPtr) {
    if (!reqPtr) return;
    ClientRequest* req = (ClientRequest*)reqPtr;
    if (req->method) free(req->method);
    if (req->path) free(req->path);
    if (req->host) free(req->host);
    if (req->body) free(req->body);
    if (req->socket != INVALID_SOCK) {
        CLOSE_SOCKET(req->socket);
    }
    delete req;
}

// ============================================================================
// Server Accept and Request Handling
// ============================================================================

// Simple HTTP request parser
static bool parseHttpRequest(const char* requestData, IncomingMessage* msg) {
    if (!requestData || !msg) return false;

    // Parse request line: METHOD /path HTTP/1.1
    const char* line = requestData;
    const char* lineEnd = strstr(line, "\r\n");
    if (!lineEnd) return false;

    // Extract method
    const char* space1 = strchr(line, ' ');
    if (!space1 || space1 > lineEnd) return false;
    size_t methodLen = space1 - line;
    msg->method = (char*)malloc(methodLen + 1);
    memcpy(msg->method, line, methodLen);
    msg->method[methodLen] = '\0';

    // Extract URL/path
    const char* space2 = strchr(space1 + 1, ' ');
    if (!space2 || space2 > lineEnd) return false;
    size_t urlLen = space2 - (space1 + 1);
    msg->url = (char*)malloc(urlLen + 1);
    memcpy(msg->url, space1 + 1, urlLen);
    msg->url[urlLen] = '\0';

    // Extract HTTP version
    const char* versionStart = space2 + 1;
    size_t versionLen = lineEnd - versionStart;
    if (msg->httpVersion) free(msg->httpVersion);
    msg->httpVersion = (char*)malloc(versionLen + 1);
    memcpy(msg->httpVersion, versionStart, versionLen);
    msg->httpVersion[versionLen] = '\0';

    // Parse headers
    line = lineEnd + 2;
    while (*line != '\r' || *(line + 1) != '\n') {
        lineEnd = strstr(line, "\r\n");
        if (!lineEnd) break;

        const char* colon = strchr(line, ':');
        if (colon && colon < lineEnd) {
            // Extract header name
            std::string name(line, colon - line);
            for (auto& c : name) c = (char)tolower(c);

            // Extract header value (skip leading spaces)
            const char* valueStart = colon + 1;
            while (*valueStart == ' ' && valueStart < lineEnd) valueStart++;
            std::string value(valueStart, lineEnd - valueStart);

            msg->headers[name] = value;
        }

        line = lineEnd + 2;
    }

    // Body would be after the blank line, but we'll keep it simple for now
    msg->complete = 1;
    return true;
}

// Accept one connection and handle one request (blocking)
// Returns 1 if request was handled, 0 if timeout, -1 on error
int nova_http_Server_acceptOne(void* serverPtr, int timeoutMs) {
    if (!serverPtr) return -1;
    Server* server = (Server*)serverPtr;

    if (server->socket == INVALID_SOCK || !server->listening) {
        return -1;
    }

    // Variables for connection handling
    struct sockaddr_in clientAddr;
#ifdef _WIN32
    int addrLen = sizeof(clientAddr);
#else
    socklen_t addrLen = sizeof(clientAddr);
#endif

#ifdef _WIN32
    // WINDOWS FIX: select() does NOT reliably detect incoming connections on Windows.
    // Use non-blocking accept() with polling instead.

    // Set socket to non-blocking mode
    u_long nonBlockingMode = 1;  // 1 = non-blocking
    if (ioctlsocket(server->socket, FIONBIO, &nonBlockingMode) != 0) {
        int err = WSAGetLastError();
        (void)err;  // Suppress unused warning when debug disabled
        HTTP_DBG("DEBUG acceptOne: Failed to set non-blocking mode, WSAGetLastError()=%d\n", err);
        return -1;
    }

    // Poll for incoming connections with timeout
    int pollInterval = 100;  // Poll every 100ms
    int elapsedMs = 0;
    int pollCount = 0;

    HTTP_DBG("DEBUG acceptOne: Starting polling loop with timeout=%dms, interval=%dms\n", timeoutMs, pollInterval);
    fflush(stderr);

    while (elapsedMs < timeoutMs) {
        socket_t clientSocket = accept(server->socket, (struct sockaddr*)&clientAddr, &addrLen);
        pollCount++;

        if (pollCount <= 3 || (pollCount % 10 == 0)) {
            HTTP_DBG("DEBUG acceptOne: Poll #%d, elapsed=%dms, accept() returned socket=%d\n",
                    pollCount, elapsedMs, (int)clientSocket);
            fflush(stderr);
        }

        if (clientSocket != INVALID_SOCK) {
            // Connection accepted successfully!
            HTTP_DBG("DEBUG acceptOne: *** CONNECTION ACCEPTED *** clientSocket=%d\n", (int)clientSocket);
            fflush(stderr);

            // Read HTTP request (simplified - read up to 8KB)
            char buffer[8192];
            int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

            if (bytesRead <= 0) {
                CLOSE_SOCKET(clientSocket);
                return -1;
            }

            buffer[bytesRead] = '\0';

            // Parse request
            IncomingMessage* req = (IncomingMessage*)nova_http_IncomingMessage_new();
            req->socket = clientSocket;

            if (!parseHttpRequest(buffer, req)) {
                nova_http_IncomingMessage_free(req);
                CLOSE_SOCKET(clientSocket);
                return -1;
            }

            // Create response
            ServerResponse* res = (ServerResponse*)nova_http_ServerResponse_new(clientSocket);

            // Call request handler
            if (server->onRequest) {
                server->onRequest(req, res);
            }

            // Ensure response is sent and finished
            if (!res->finished) {
                nova_http_ServerResponse_end(res, nullptr, 0);
            }

            // Check for Keep-Alive support (HTTP/1.1 default)
            bool keepAlive = true;
            bool isHttp11 = (req->httpVersion && strcmp(req->httpVersion, "1.1") == 0);

            // Check Connection header
            char* connectionHeader = nova_http_IncomingMessage_getHeader(req, "connection");
            if (connectionHeader) {
                std::string conn = connectionHeader;
                for (auto& c : conn) c = (char)tolower(c);

                if (isHttp11) {
                    // HTTP/1.1: keep-alive by default unless "close" specified
                    keepAlive = (conn.find("close") == std::string::npos);
                } else {
                    // HTTP/1.0: close by default unless "keep-alive" specified
                    keepAlive = (conn.find("keep-alive") != std::string::npos);
                }
                free(connectionHeader);
            } else {
                // No Connection header: use HTTP version default
                keepAlive = isHttp11;  // HTTP/1.1 keeps alive by default
            }

            // Clean up
            nova_http_IncomingMessage_free(req);
            nova_http_ServerResponse_free(res);

            if (!keepAlive) {
                // Client wants to close connection
                CLOSE_SOCKET(clientSocket);
                return 1;
            }

            // Keep-Alive: Set socket timeout and handle multiple requests
            DWORD timeout = 5000;  // 5 seconds idle timeout
            setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

            // Keep socket in blocking mode for recv()
            u_long blockingMode = 0;
            ioctlsocket(clientSocket, FIONBIO, &blockingMode);

            // Loop to handle multiple requests on this connection
            int requestsOnConnection = 1;  // Already handled first request
            const int maxRequestsPerConnection = 1000;  // Prevent infinite loops

            while (keepAlive && requestsOnConnection < maxRequestsPerConnection) {
                // Read next request
                char buffer2[8192];
                int bytesRead2 = recv(clientSocket, buffer2, sizeof(buffer2) - 1, 0);

                if (bytesRead2 <= 0) {
                    // Timeout or client closed connection
                    break;
                }

                buffer2[bytesRead2] = '\0';

                // Parse request
                IncomingMessage* req2 = (IncomingMessage*)nova_http_IncomingMessage_new();
                req2->socket = clientSocket;

                if (!parseHttpRequest(buffer2, req2)) {
                    nova_http_IncomingMessage_free(req2);
                    break;
                }

                // Create response
                ServerResponse* res2 = (ServerResponse*)nova_http_ServerResponse_new(clientSocket);

                // Call request handler
                if (server->onRequest) {
                    server->onRequest(req2, res2);
                }

                // Ensure response is sent
                if (!res2->finished) {
                    nova_http_ServerResponse_end(res2, nullptr, 0);
                }

                // Check if we should keep connection alive for next request
                bool isHttp11_2 = (req2->httpVersion && strcmp(req2->httpVersion, "1.1") == 0);
                char* connectionHeader2 = nova_http_IncomingMessage_getHeader(req2, "connection");

                if (connectionHeader2) {
                    std::string conn2 = connectionHeader2;
                    for (auto& c : conn2) c = (char)tolower(c);

                    if (isHttp11_2) {
                        keepAlive = (conn2.find("close") == std::string::npos);
                    } else {
                        keepAlive = (conn2.find("keep-alive") != std::string::npos);
                    }
                    free(connectionHeader2);
                } else {
                    keepAlive = isHttp11_2;
                }

                // Clean up
                nova_http_IncomingMessage_free(req2);
                nova_http_ServerResponse_free(res2);

                requestsOnConnection++;
            }

            // Close connection after keep-alive loop ends
            CLOSE_SOCKET(clientSocket);
            return requestsOnConnection;  // Return number of requests handled
        }

        // Check error
        int err = WSAGetLastError();
        (void)err;  // Suppress unused warning when debug disabled
        if (err != WSAEWOULDBLOCK) {
            // Real error (not just "no connection pending")
            HTTP_DBG("DEBUG acceptOne: accept() error, WSAGetLastError()=%d\n", err);
            return -1;
        }

        // No connection yet, sleep and retry
        Sleep(pollInterval);
        elapsedMs += pollInterval;
    }

    // Timeout - no connection within the timeout period
    HTTP_DBG("DEBUG acceptOne: Polling loop timeout after %d polls, %dms elapsed\n", pollCount, elapsedMs);
    fflush(stderr);
    return 0;

#else
    // POSIX systems: use select() which works correctly for listening sockets
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(server->socket, &readfds);

    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    int selectResult = select((int)server->socket + 1, &readfds, NULL, NULL, &tv);

    if (selectResult < 0) {
        return -1;
    }

    if (selectResult == 0) {
        return 0;  // Timeout
    }

    // Connection available
    socket_t clientSocket = accept(server->socket, (struct sockaddr*)&clientAddr, &addrLen);

    if (clientSocket == INVALID_SOCK) {
#ifdef _WIN32
        int err = WSAGetLastError();
        (void)err;  // Suppress unused warning when debug disabled
        HTTP_DBG("DEBUG acceptOne: accept() failed, WSAGetLastError()=%d\n", err);
#else
        HTTP_DBG("DEBUG acceptOne: accept() failed, errno=%d\n", errno);
#endif
        return -1;
    }

    HTTP_DBG("DEBUG acceptOne: accept() successful, clientSocket=%d\n", (int)clientSocket);
    fflush(stderr);

    // Read HTTP request (simplified - read up to 8KB)
    char buffer[8192];
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        CLOSE_SOCKET(clientSocket);
        return -1;
    }

    buffer[bytesRead] = '\0';

    // Parse request
    IncomingMessage* req = (IncomingMessage*)nova_http_IncomingMessage_new();
    req->socket = clientSocket;

    if (!parseHttpRequest(buffer, req)) {
        nova_http_IncomingMessage_free(req);
        CLOSE_SOCKET(clientSocket);
        return -1;
    }

    // Create response
    ServerResponse* res = (ServerResponse*)nova_http_ServerResponse_new(clientSocket);

    // Call request handler
    HTTP_DBG("DEBUG acceptOne: About to call request handler, onRequest=%p\n", (void*)server->onRequest);
    if (server->onRequest) {
        HTTP_DBG("DEBUG acceptOne: Calling request handler with req=%p, res=%p\n", (void*)req, (void*)res);
        server->onRequest(req, res);  // Node.js-compatible: callback(req, res)
        HTTP_DBG("DEBUG acceptOne: Request handler returned\n");
    } else {
        HTTP_DBG("DEBUG acceptOne: No request handler registered!\n");
    }

    // Ensure response is sent and finished
    if (!res->finished) {
        nova_http_ServerResponse_end(res, nullptr, 0);
    }

    // Check for Keep-Alive support (HTTP/1.1 default)
    bool keepAlive = true;
    bool isHttp11 = (req->httpVersion && strcmp(req->httpVersion, "1.1") == 0);

    // Check Connection header
    char* connectionHeader = nova_http_IncomingMessage_getHeader(req, "connection");
    if (connectionHeader) {
        std::string conn = connectionHeader;
        for (auto& c : conn) c = (char)tolower(c);

        if (isHttp11) {
            // HTTP/1.1: keep-alive by default unless "close" specified
            keepAlive = (conn.find("close") == std::string::npos);
        } else {
            // HTTP/1.0: close by default unless "keep-alive" specified
            keepAlive = (conn.find("keep-alive") != std::string::npos);
        }
        free(connectionHeader);
    } else {
        // No Connection header: use HTTP version default
        keepAlive = isHttp11;  // HTTP/1.1 keeps alive by default
    }

    // Clean up request/response objects
    nova_http_IncomingMessage_free(req);
    nova_http_ServerResponse_free(res);

    if (!keepAlive) {
        // Client wants to close connection
        CLOSE_SOCKET(clientSocket);
        return 1;
    }

    // Keep-Alive: Set socket timeout and handle multiple requests
    struct timeval tv;
    tv.tv_sec = 5;   // 5 seconds idle timeout
    tv.tv_usec = 0;
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Loop to handle multiple requests on this connection
    int requestsOnConnection = 1;  // Already handled first request
    const int maxRequestsPerConnection = 1000;  // Prevent infinite loops

    while (keepAlive && requestsOnConnection < maxRequestsPerConnection) {
        // Read next request
        char buffer2[8192];
        int bytesRead2 = recv(clientSocket, buffer2, sizeof(buffer2) - 1, 0);

        if (bytesRead2 <= 0) {
            // Timeout or client closed connection
            break;
        }

        buffer2[bytesRead2] = '\0';

        // Parse request
        IncomingMessage* req2 = (IncomingMessage*)nova_http_IncomingMessage_new();
        req2->socket = clientSocket;

        if (!parseHttpRequest(buffer2, req2)) {
            nova_http_IncomingMessage_free(req2);
            break;
        }

        // Create response
        ServerResponse* res2 = (ServerResponse*)nova_http_ServerResponse_new(clientSocket);

        // Call request handler
        if (server->onRequest) {
            server->onRequest(req2, res2);
        }

        // Ensure response is sent
        if (!res2->finished) {
            nova_http_ServerResponse_end(res2, nullptr, 0);
        }

        // Check if we should keep connection alive for next request
        bool isHttp11_2 = (req2->httpVersion && strcmp(req2->httpVersion, "1.1") == 0);
        char* connectionHeader2 = nova_http_IncomingMessage_getHeader(req2, "connection");

        if (connectionHeader2) {
            std::string conn2 = connectionHeader2;
            for (auto& c : conn2) c = (char)tolower(c);

            if (isHttp11_2) {
                keepAlive = (conn2.find("close") == std::string::npos);
            } else {
                keepAlive = (conn2.find("keep-alive") != std::string::npos);
            }
            free(connectionHeader2);
        } else {
            keepAlive = isHttp11_2;
        }

        // Clean up
        nova_http_IncomingMessage_free(req2);
        nova_http_ServerResponse_free(res2);

        requestsOnConnection++;
    }

    // Close connection after keep-alive loop ends
    CLOSE_SOCKET(clientSocket);
    return requestsOnConnection;  // Return number of requests handled
#endif
}

// Run server event loop (handles multiple requests until stopped)
// This is a blocking call - returns number of requests handled, or -1 on error
int nova_http_Server_run(void* serverPtr, int maxRequests) {
    HTTP_DBG("DEBUG nova_http_Server_run: Called with maxRequests=%d\n", maxRequests);

    if (!serverPtr) {
        HTTP_DBG("DEBUG nova_http_Server_run: serverPtr is NULL\n");
        return -1;
    }
    Server* server = (Server*)serverPtr;

    HTTP_DBG("DEBUG nova_http_Server_run: socket=%d, listening=%d, INVALID_SOCK=%d\n",
            (int)server->socket, server->listening, (int)INVALID_SOCK);

    if (server->socket == INVALID_SOCK || !server->listening) {
        HTTP_DBG("DEBUG nova_http_Server_run: Early exit - socket invalid or not listening\n");
        return -1;
    }

    HTTP_DBG("DEBUG nova_http_Server_run: Entering event loop\n");
    int requestsHandled = 0;

    while (server->listening && (maxRequests == 0 || requestsHandled < maxRequests)) {
        HTTP_DBG("DEBUG nova_http_Server_run: *** ABOUT TO CALL nova_http_Server_acceptOne() ***\n");
        int result = nova_http_Server_acceptOne(serverPtr, 5000);  // 5000ms (5 second) timeout
        HTTP_DBG("DEBUG nova_http_Server_run: *** RETURNED FROM nova_http_Server_acceptOne(), result=%d ***\n", result);

        if (result > 0) {
            requestsHandled++;
            HTTP_DBG("DEBUG nova_http_Server_run: Handled request #%d\n", requestsHandled);
        } else if (result < 0) {
            HTTP_DBG("DEBUG nova_http_Server_run: acceptOne error, breaking loop\n");
            break;  // Error
        }
        // result == 0 means timeout, continue loop
    }

    HTTP_DBG("DEBUG nova_http_Server_run: Exiting, handled %d requests\n", requestsHandled);
    return requestsHandled;
}

// ============================================================================
// Utility Functions
// ============================================================================

void nova_http_freeStringArray(char** arr, int count) {
    if (arr) {
        for (int i = 0; i < count; i++) {
            if (arr[i]) free(arr[i]);
        }
        free(arr);
    }
}

void nova_http_cleanup() {
    if (globalAgent) {
        nova_http_Agent_free(globalAgent);
        globalAgent = nullptr;
    }

#ifdef _WIN32
    if (wsaInitialized) {
        WSACleanup();
        wsaInitialized = false;
    }
#endif
}

} // extern "C"

} // namespace http
} // namespace runtime
} // namespace nova
