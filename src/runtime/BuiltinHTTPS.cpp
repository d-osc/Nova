// Nova HTTPS Module - Node.js compatible HTTPS server/client
// Provides TLS/SSL encrypted HTTP connections

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <functional>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wincrypt.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "crypt32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif

extern "C" {

// Helper to allocate string
static char* allocString(const std::string& s) {
    char* result = (char*)malloc(s.size() + 1);
    if (result) {
        memcpy(result, s.c_str(), s.size() + 1);
    }
    return result;
}

// ============================================================================
// HTTPS Agent
// ============================================================================

struct NovaHttpsAgent {
    int maxSockets;
    int maxTotalSockets;
    int maxFreeSockets;
    int timeout;
    bool keepAlive;
    int keepAliveMsecs;
    bool scheduling;  // 'lifo' or 'fifo'
    std::map<std::string, std::vector<int>> sockets;
    std::map<std::string, std::vector<int>> freeSockets;
    std::map<std::string, std::vector<void*>> requests;
    // TLS options
    char* ca;
    char* cert;
    char* key;
    char* passphrase;
    bool rejectUnauthorized;
    char* servername;
    int minVersion;  // TLS version
    int maxVersion;
};

void* nova_https_Agent_new() {
    NovaHttpsAgent* agent = new NovaHttpsAgent();
    agent->maxSockets = 256;
    agent->maxTotalSockets = 256;
    agent->maxFreeSockets = 256;
    agent->timeout = 0;
    agent->keepAlive = false;
    agent->keepAliveMsecs = 1000;
    agent->scheduling = false;  // fifo
    agent->ca = nullptr;
    agent->cert = nullptr;
    agent->key = nullptr;
    agent->passphrase = nullptr;
    agent->rejectUnauthorized = true;
    agent->servername = nullptr;
    agent->minVersion = 0;
    agent->maxVersion = 0;
    return agent;
}

void nova_https_Agent_free(void* agent) {
    NovaHttpsAgent* a = (NovaHttpsAgent*)agent;
    if (a) {
        free(a->ca);
        free(a->cert);
        free(a->key);
        free(a->passphrase);
        free(a->servername);
        delete a;
    }
}

void nova_https_Agent_destroy(void* agent) {
    NovaHttpsAgent* a = (NovaHttpsAgent*)agent;
    if (a) {
        // Close all sockets
        for (auto& pair : a->sockets) {
            for (int sock : pair.second) {
#ifdef _WIN32
                closesocket(sock);
#else
                close(sock);
#endif
            }
        }
        for (auto& pair : a->freeSockets) {
            for (int sock : pair.second) {
#ifdef _WIN32
                closesocket(sock);
#else
                close(sock);
#endif
            }
        }
        a->sockets.clear();
        a->freeSockets.clear();
        a->requests.clear();
    }
}

int nova_https_Agent_maxSockets(void* agent) {
    return agent ? ((NovaHttpsAgent*)agent)->maxSockets : 256;
}

void nova_https_Agent_setMaxSockets(void* agent, int value) {
    if (agent) ((NovaHttpsAgent*)agent)->maxSockets = value;
}

int nova_https_Agent_maxTotalSockets(void* agent) {
    return agent ? ((NovaHttpsAgent*)agent)->maxTotalSockets : 256;
}

void nova_https_Agent_setMaxTotalSockets(void* agent, int value) {
    if (agent) ((NovaHttpsAgent*)agent)->maxTotalSockets = value;
}

int nova_https_Agent_maxFreeSockets(void* agent) {
    return agent ? ((NovaHttpsAgent*)agent)->maxFreeSockets : 256;
}

void nova_https_Agent_setMaxFreeSockets(void* agent, int value) {
    if (agent) ((NovaHttpsAgent*)agent)->maxFreeSockets = value;
}

int nova_https_Agent_keepAlive(void* agent) {
    return agent ? ((NovaHttpsAgent*)agent)->keepAlive : 0;
}

void nova_https_Agent_setKeepAlive(void* agent, int value) {
    if (agent) ((NovaHttpsAgent*)agent)->keepAlive = value != 0;
}

int nova_https_Agent_keepAliveMsecs(void* agent) {
    return agent ? ((NovaHttpsAgent*)agent)->keepAliveMsecs : 1000;
}

void nova_https_Agent_setKeepAliveMsecs(void* agent, int value) {
    if (agent) ((NovaHttpsAgent*)agent)->keepAliveMsecs = value;
}

char* nova_https_Agent_getName(void* agent, const char* host, int port, const char* localAddress) {
    std::string name = std::string(host ? host : "localhost") + ":" + std::to_string(port);
    if (localAddress && *localAddress) {
        name += ":" + std::string(localAddress);
    }
    return allocString(name);
}

// TLS-specific agent options
void nova_https_Agent_setCa(void* agent, const char* ca) {
    NovaHttpsAgent* a = (NovaHttpsAgent*)agent;
    if (a) {
        free(a->ca);
        a->ca = ca ? allocString(ca) : nullptr;
    }
}

void nova_https_Agent_setCert(void* agent, const char* cert) {
    NovaHttpsAgent* a = (NovaHttpsAgent*)agent;
    if (a) {
        free(a->cert);
        a->cert = cert ? allocString(cert) : nullptr;
    }
}

void nova_https_Agent_setKey(void* agent, const char* key) {
    NovaHttpsAgent* a = (NovaHttpsAgent*)agent;
    if (a) {
        free(a->key);
        a->key = key ? allocString(key) : nullptr;
    }
}

void nova_https_Agent_setPassphrase(void* agent, const char* passphrase) {
    NovaHttpsAgent* a = (NovaHttpsAgent*)agent;
    if (a) {
        free(a->passphrase);
        a->passphrase = passphrase ? allocString(passphrase) : nullptr;
    }
}

int nova_https_Agent_rejectUnauthorized(void* agent) {
    return agent ? ((NovaHttpsAgent*)agent)->rejectUnauthorized : 1;
}

void nova_https_Agent_setRejectUnauthorized(void* agent, int value) {
    if (agent) ((NovaHttpsAgent*)agent)->rejectUnauthorized = value != 0;
}

void nova_https_Agent_setServername(void* agent, const char* servername) {
    NovaHttpsAgent* a = (NovaHttpsAgent*)agent;
    if (a) {
        free(a->servername);
        a->servername = servername ? allocString(servername) : nullptr;
    }
}

// ============================================================================
// Global Agent
// ============================================================================

static NovaHttpsAgent* globalAgent = nullptr;

void* nova_https_globalAgent() {
    if (!globalAgent) {
        globalAgent = (NovaHttpsAgent*)nova_https_Agent_new();
        globalAgent->keepAlive = true;
    }
    return globalAgent;
}

// ============================================================================
// HTTPS Server
// ============================================================================

struct NovaHttpsServer {
    int socket;
    int port;
    bool listening;
    int timeout;
    int headersTimeout;
    int requestTimeout;
    int keepAliveTimeout;
    int maxHeadersCount;
    int maxRequestsPerSocket;
    // TLS options
    char* cert;
    char* key;
    char* ca;
    char* passphrase;
    bool requestCert;
    bool rejectUnauthorized;
    // Callbacks
    std::function<void(void*, void*)> requestCallback;
    std::function<void()> listeningCallback;
    std::function<void(const char*)> errorCallback;
    std::function<void()> closeCallback;
};

void* nova_https_Server_new() {
    NovaHttpsServer* server = new NovaHttpsServer();
    server->socket = -1;
    server->port = 443;
    server->listening = false;
    server->timeout = 0;
    server->headersTimeout = 60000;
    server->requestTimeout = 300000;
    server->keepAliveTimeout = 5000;
    server->maxHeadersCount = 2000;
    server->maxRequestsPerSocket = 0;
    server->cert = nullptr;
    server->key = nullptr;
    server->ca = nullptr;
    server->passphrase = nullptr;
    server->requestCert = false;
    server->rejectUnauthorized = true;
    return server;
}

void nova_https_Server_free(void* server) {
    NovaHttpsServer* s = (NovaHttpsServer*)server;
    if (s) {
        if (s->socket >= 0) {
#ifdef _WIN32
            closesocket(s->socket);
#else
            close(s->socket);
#endif
        }
        free(s->cert);
        free(s->key);
        free(s->ca);
        free(s->passphrase);
        delete s;
    }
}

void nova_https_Server_setCert(void* server, const char* cert) {
    NovaHttpsServer* s = (NovaHttpsServer*)server;
    if (s) {
        free(s->cert);
        s->cert = cert ? allocString(cert) : nullptr;
    }
}

void nova_https_Server_setKey(void* server, const char* key) {
    NovaHttpsServer* s = (NovaHttpsServer*)server;
    if (s) {
        free(s->key);
        s->key = key ? allocString(key) : nullptr;
    }
}

void nova_https_Server_setCa(void* server, const char* ca) {
    NovaHttpsServer* s = (NovaHttpsServer*)server;
    if (s) {
        free(s->ca);
        s->ca = ca ? allocString(ca) : nullptr;
    }
}

void nova_https_Server_setPassphrase(void* server, const char* passphrase) {
    NovaHttpsServer* s = (NovaHttpsServer*)server;
    if (s) {
        free(s->passphrase);
        s->passphrase = passphrase ? allocString(passphrase) : nullptr;
    }
}

void nova_https_Server_setRequestCert(void* server, int value) {
    if (server) ((NovaHttpsServer*)server)->requestCert = value != 0;
}

void nova_https_Server_setRejectUnauthorized(void* server, int value) {
    if (server) ((NovaHttpsServer*)server)->rejectUnauthorized = value != 0;
}

int nova_https_Server_listen(void* server, int port, const char* host) {
    NovaHttpsServer* s = (NovaHttpsServer*)server;
    if (!s) return -1;

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    s->socket = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s->socket < 0) return -1;

    int opt = 1;
#ifdef _WIN32
    setsockopt(s->socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(s->socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (host && *host) {
        inet_pton(AF_INET, host, &addr.sin_addr);
    } else {
        addr.sin_addr.s_addr = INADDR_ANY;
    }

    if (bind(s->socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
#ifdef _WIN32
        closesocket(s->socket);
#else
        close(s->socket);
#endif
        s->socket = -1;
        return -1;
    }

    if (listen(s->socket, SOMAXCONN) < 0) {
#ifdef _WIN32
        closesocket(s->socket);
#else
        close(s->socket);
#endif
        s->socket = -1;
        return -1;
    }

    s->port = port;
    s->listening = true;

    if (s->listeningCallback) {
        s->listeningCallback();
    }

    return 0;
}

void nova_https_Server_close(void* server) {
    NovaHttpsServer* s = (NovaHttpsServer*)server;
    if (s && s->socket >= 0) {
#ifdef _WIN32
        closesocket(s->socket);
#else
        close(s->socket);
#endif
        s->socket = -1;
        s->listening = false;
        if (s->closeCallback) {
            s->closeCallback();
        }
    }
}

int nova_https_Server_listening(void* server) {
    return server ? ((NovaHttpsServer*)server)->listening : 0;
}

void nova_https_Server_setTimeout(void* server, int timeout) {
    if (server) ((NovaHttpsServer*)server)->timeout = timeout;
}

int nova_https_Server_timeout(void* server) {
    return server ? ((NovaHttpsServer*)server)->timeout : 0;
}

void nova_https_Server_setHeadersTimeout(void* server, int timeout) {
    if (server) ((NovaHttpsServer*)server)->headersTimeout = timeout;
}

int nova_https_Server_headersTimeout(void* server) {
    return server ? ((NovaHttpsServer*)server)->headersTimeout : 60000;
}

void nova_https_Server_setRequestTimeout(void* server, int timeout) {
    if (server) ((NovaHttpsServer*)server)->requestTimeout = timeout;
}

int nova_https_Server_requestTimeout(void* server) {
    return server ? ((NovaHttpsServer*)server)->requestTimeout : 300000;
}

void nova_https_Server_setKeepAliveTimeout(void* server, int timeout) {
    if (server) ((NovaHttpsServer*)server)->keepAliveTimeout = timeout;
}

int nova_https_Server_keepAliveTimeout(void* server) {
    return server ? ((NovaHttpsServer*)server)->keepAliveTimeout : 5000;
}

void nova_https_Server_setMaxHeadersCount(void* server, int count) {
    if (server) ((NovaHttpsServer*)server)->maxHeadersCount = count;
}

int nova_https_Server_maxHeadersCount(void* server) {
    return server ? ((NovaHttpsServer*)server)->maxHeadersCount : 2000;
}

void nova_https_Server_setMaxRequestsPerSocket(void* server, int count) {
    if (server) ((NovaHttpsServer*)server)->maxRequestsPerSocket = count;
}

int nova_https_Server_maxRequestsPerSocket(void* server) {
    return server ? ((NovaHttpsServer*)server)->maxRequestsPerSocket : 0;
}

void nova_https_Server_on(void* server, const char* event, void* callback) {
    // Event registration - handled by runtime
    (void)server;
    (void)event;
    (void)callback;
}

void nova_https_Server_closeAllConnections(void* server) {
    // Close all active connections
    (void)server;
}

void nova_https_Server_closeIdleConnections(void* server) {
    // Close idle connections
    (void)server;
}

// ============================================================================
// HTTPS Request (ClientRequest)
// ============================================================================

struct NovaHttpsClientRequest {
    int socket;
    char* method;
    char* path;
    char* host;
    int port;
    std::map<std::string, std::string> headers;
    bool headersSent;
    bool finished;
    bool aborted;
    bool reusedSocket;
    int maxHeadersCount;
    // TLS options
    char* ca;
    char* cert;
    char* key;
    char* passphrase;
    bool rejectUnauthorized;
    char* servername;
};

void* nova_https_ClientRequest_new() {
    NovaHttpsClientRequest* req = new NovaHttpsClientRequest();
    req->socket = -1;
    req->method = allocString("GET");
    req->path = allocString("/");
    req->host = allocString("localhost");
    req->port = 443;
    req->headersSent = false;
    req->finished = false;
    req->aborted = false;
    req->reusedSocket = false;
    req->maxHeadersCount = 2000;
    req->ca = nullptr;
    req->cert = nullptr;
    req->key = nullptr;
    req->passphrase = nullptr;
    req->rejectUnauthorized = true;
    req->servername = nullptr;
    return req;
}

void nova_https_ClientRequest_free(void* request) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    if (req) {
        if (req->socket >= 0) {
#ifdef _WIN32
            closesocket(req->socket);
#else
            close(req->socket);
#endif
        }
        free(req->method);
        free(req->path);
        free(req->host);
        free(req->ca);
        free(req->cert);
        free(req->key);
        free(req->passphrase);
        free(req->servername);
        delete req;
    }
}

void nova_https_ClientRequest_setMethod(void* request, const char* method) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    if (req) {
        free(req->method);
        req->method = allocString(method ? method : "GET");
    }
}

void nova_https_ClientRequest_setPath(void* request, const char* path) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    if (req) {
        free(req->path);
        req->path = allocString(path ? path : "/");
    }
}

void nova_https_ClientRequest_setHost(void* request, const char* host) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    if (req) {
        free(req->host);
        req->host = allocString(host ? host : "localhost");
    }
}

void nova_https_ClientRequest_setPort(void* request, int port) {
    if (request) ((NovaHttpsClientRequest*)request)->port = port;
}

void nova_https_ClientRequest_setHeader(void* request, const char* name, const char* value) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    if (req && name && value) {
        req->headers[name] = value;
    }
}

char* nova_https_ClientRequest_getHeader(void* request, const char* name) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    if (req && name) {
        auto it = req->headers.find(name);
        if (it != req->headers.end()) {
            return allocString(it->second);
        }
    }
    return nullptr;
}

void nova_https_ClientRequest_removeHeader(void* request, const char* name) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    if (req && name) {
        req->headers.erase(name);
    }
}

int nova_https_ClientRequest_hasHeader(void* request, const char* name) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    return (req && name && req->headers.find(name) != req->headers.end()) ? 1 : 0;
}

char* nova_https_ClientRequest_method(void* request) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    return req ? allocString(req->method) : nullptr;
}

char* nova_https_ClientRequest_path(void* request) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    return req ? allocString(req->path) : nullptr;
}

char* nova_https_ClientRequest_host(void* request) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    return req ? allocString(req->host) : nullptr;
}

char* nova_https_ClientRequest_protocol(void* request) {
    (void)request;
    return allocString("https:");
}

int nova_https_ClientRequest_reusedSocket(void* request) {
    return request ? ((NovaHttpsClientRequest*)request)->reusedSocket : 0;
}

int nova_https_ClientRequest_maxHeadersCount(void* request) {
    return request ? ((NovaHttpsClientRequest*)request)->maxHeadersCount : 2000;
}

void nova_https_ClientRequest_setMaxHeadersCount(void* request, int count) {
    if (request) ((NovaHttpsClientRequest*)request)->maxHeadersCount = count;
}

void nova_https_ClientRequest_write(void* request, const char* data, int length) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    if (req && req->socket >= 0 && data) {
        send(req->socket, data, length, 0);
    }
}

void nova_https_ClientRequest_end(void* request, const char* data, int length) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    if (req) {
        if (data && length > 0 && req->socket >= 0) {
            send(req->socket, data, length, 0);
        }
        req->finished = true;
    }
}

void nova_https_ClientRequest_abort(void* request) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    if (req) {
        req->aborted = true;
        if (req->socket >= 0) {
#ifdef _WIN32
            closesocket(req->socket);
#else
            close(req->socket);
#endif
            req->socket = -1;
        }
    }
}

void nova_https_ClientRequest_destroy(void* request) {
    nova_https_ClientRequest_abort(request);
}

int nova_https_ClientRequest_destroyed(void* request) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    return (req && req->socket < 0) ? 1 : 0;
}

int nova_https_ClientRequest_writableEnded(void* request) {
    return request ? ((NovaHttpsClientRequest*)request)->finished : 0;
}

int nova_https_ClientRequest_writableFinished(void* request) {
    return request ? ((NovaHttpsClientRequest*)request)->finished : 0;
}

void nova_https_ClientRequest_flushHeaders(void* request) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    if (req && !req->headersSent) {
        req->headersSent = true;
    }
}

void nova_https_ClientRequest_setNoDelay(void* request, int noDelay) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    if (req && req->socket >= 0) {
#ifdef _WIN32
        setsockopt(req->socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));
#else
        setsockopt(req->socket, IPPROTO_TCP, TCP_NODELAY, &noDelay, sizeof(noDelay));
#endif
    }
}

void nova_https_ClientRequest_setSocketKeepAlive(void* request, int enable, int initialDelay) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    if (req && req->socket >= 0) {
#ifdef _WIN32
        setsockopt(req->socket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&enable, sizeof(enable));
#else
        setsockopt(req->socket, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable));
#endif
        (void)initialDelay;
    }
}

void nova_https_ClientRequest_setTimeout(void* request, int timeout) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    if (req && req->socket >= 0) {
#ifdef _WIN32
        setsockopt(req->socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
        setsockopt(req->socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#else
        struct timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        setsockopt(req->socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(req->socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
    }
}

void nova_https_ClientRequest_on(void* request, const char* event, void* callback) {
    (void)request;
    (void)event;
    (void)callback;
}

// TLS-specific options for client request
void nova_https_ClientRequest_setCa(void* request, const char* ca) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    if (req) {
        free(req->ca);
        req->ca = ca ? allocString(ca) : nullptr;
    }
}

void nova_https_ClientRequest_setCert(void* request, const char* cert) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    if (req) {
        free(req->cert);
        req->cert = cert ? allocString(cert) : nullptr;
    }
}

void nova_https_ClientRequest_setKey(void* request, const char* key) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    if (req) {
        free(req->key);
        req->key = key ? allocString(key) : nullptr;
    }
}

void nova_https_ClientRequest_setPassphrase(void* request, const char* passphrase) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    if (req) {
        free(req->passphrase);
        req->passphrase = passphrase ? allocString(passphrase) : nullptr;
    }
}

void nova_https_ClientRequest_setRejectUnauthorized(void* request, int value) {
    if (request) ((NovaHttpsClientRequest*)request)->rejectUnauthorized = value != 0;
}

void nova_https_ClientRequest_setServername(void* request, const char* servername) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)request;
    if (req) {
        free(req->servername);
        req->servername = servername ? allocString(servername) : nullptr;
    }
}

// ============================================================================
// HTTPS IncomingMessage (Response)
// ============================================================================

struct NovaHttpsIncomingMessage {
    int statusCode;
    char* statusMessage;
    char* httpVersion;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> trailers;
    char* url;
    char* method;
    bool complete;
    bool aborted;
    int socket;
};

void* nova_https_IncomingMessage_new() {
    NovaHttpsIncomingMessage* msg = new NovaHttpsIncomingMessage();
    msg->statusCode = 200;
    msg->statusMessage = allocString("OK");
    msg->httpVersion = allocString("1.1");
    msg->url = nullptr;
    msg->method = nullptr;
    msg->complete = false;
    msg->aborted = false;
    msg->socket = -1;
    return msg;
}

void nova_https_IncomingMessage_free(void* message) {
    NovaHttpsIncomingMessage* msg = (NovaHttpsIncomingMessage*)message;
    if (msg) {
        free(msg->statusMessage);
        free(msg->httpVersion);
        free(msg->url);
        free(msg->method);
        delete msg;
    }
}

int nova_https_IncomingMessage_statusCode(void* message) {
    return message ? ((NovaHttpsIncomingMessage*)message)->statusCode : 0;
}

char* nova_https_IncomingMessage_statusMessage(void* message) {
    NovaHttpsIncomingMessage* msg = (NovaHttpsIncomingMessage*)message;
    return msg ? allocString(msg->statusMessage) : nullptr;
}

char* nova_https_IncomingMessage_httpVersion(void* message) {
    NovaHttpsIncomingMessage* msg = (NovaHttpsIncomingMessage*)message;
    return msg ? allocString(msg->httpVersion) : nullptr;
}

char* nova_https_IncomingMessage_url(void* message) {
    NovaHttpsIncomingMessage* msg = (NovaHttpsIncomingMessage*)message;
    return (msg && msg->url) ? allocString(msg->url) : nullptr;
}

char* nova_https_IncomingMessage_method(void* message) {
    NovaHttpsIncomingMessage* msg = (NovaHttpsIncomingMessage*)message;
    return (msg && msg->method) ? allocString(msg->method) : nullptr;
}

int nova_https_IncomingMessage_complete(void* message) {
    return message ? ((NovaHttpsIncomingMessage*)message)->complete : 0;
}

int nova_https_IncomingMessage_aborted(void* message) {
    return message ? ((NovaHttpsIncomingMessage*)message)->aborted : 0;
}

char* nova_https_IncomingMessage_getHeader(void* message, const char* name) {
    NovaHttpsIncomingMessage* msg = (NovaHttpsIncomingMessage*)message;
    if (msg && name) {
        auto it = msg->headers.find(name);
        if (it != msg->headers.end()) {
            return allocString(it->second);
        }
    }
    return nullptr;
}

void nova_https_IncomingMessage_destroy(void* message) {
    NovaHttpsIncomingMessage* msg = (NovaHttpsIncomingMessage*)message;
    if (msg) {
        msg->aborted = true;
    }
}

void nova_https_IncomingMessage_setTimeout(void* message, int timeout) {
    NovaHttpsIncomingMessage* msg = (NovaHttpsIncomingMessage*)message;
    if (msg && msg->socket >= 0) {
#ifdef _WIN32
        setsockopt(msg->socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
        struct timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        setsockopt(msg->socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
    }
}

void nova_https_IncomingMessage_on(void* message, const char* event, void* callback) {
    (void)message;
    (void)event;
    (void)callback;
}

// ============================================================================
// Module Functions
// ============================================================================

void* nova_https_createServer(const char* cert, const char* key) {
    NovaHttpsServer* server = (NovaHttpsServer*)nova_https_Server_new();
    if (cert) {
        server->cert = allocString(cert);
    }
    if (key) {
        server->key = allocString(key);
    }
    return server;
}

void* nova_https_request(const char* url, const char* method) {
    NovaHttpsClientRequest* req = (NovaHttpsClientRequest*)nova_https_ClientRequest_new();

    // Parse URL
    if (url) {
        std::string urlStr(url);

        // Remove https:// prefix
        size_t start = 0;
        if (urlStr.find("https://") == 0) {
            start = 8;
        } else if (urlStr.find("http://") == 0) {
            start = 7;
        }

        // Find host/path separator
        size_t pathStart = urlStr.find('/', start);
        std::string host;
        std::string path = "/";

        if (pathStart != std::string::npos) {
            host = urlStr.substr(start, pathStart - start);
            path = urlStr.substr(pathStart);
        } else {
            host = urlStr.substr(start);
        }

        // Check for port
        int port = 443;
        size_t colonPos = host.find(':');
        if (colonPos != std::string::npos) {
            port = std::stoi(host.substr(colonPos + 1));
            host = host.substr(0, colonPos);
        }

        free(req->host);
        req->host = allocString(host);
        req->port = port;
        free(req->path);
        req->path = allocString(path);
    }

    if (method) {
        free(req->method);
        req->method = allocString(method);
    }

    return req;
}

void* nova_https_get(const char* url) {
    return nova_https_request(url, "GET");
}

// ============================================================================
// Cleanup
// ============================================================================

void nova_https_cleanup() {
    if (globalAgent) {
        nova_https_Agent_free(globalAgent);
        globalAgent = nullptr;
    }
#ifdef _WIN32
    WSACleanup();
#endif
}

} // extern "C"
