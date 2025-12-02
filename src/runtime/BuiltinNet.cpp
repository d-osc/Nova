// Nova Net Module - Node.js compatible TCP/IPC networking
// Provides net.Server, net.Socket, and related utilities

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <functional>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#endif

// ============================================================================
// Internal structures (outside extern "C")
// ============================================================================

struct NovaSocket {
    int fd;
    char* remoteAddress;
    char* remoteFamily;
    int remotePort;
    char* localAddress;
    char* localFamily;
    int localPort;
    int64_t bytesRead;
    int64_t bytesWritten;
    bool connecting;
    bool destroyed;
    bool pending;
    bool readable;
    bool writable;
    int timeout;
    bool allowHalfOpen;
    std::map<std::string, void*> eventHandlers;
};

struct NovaServer {
    int fd;
    bool listening;
    int maxConnections;
    int connections;
    char* address;
    int port;
    char* family;
    std::vector<NovaSocket*> clients;
    std::map<std::string, void*> eventHandlers;
};

struct NovaBlockList {
    std::set<std::string> rules;
};

struct NovaSocketAddress {
    char* address;
    char* family;
    int port;
    int flowlabel;
};

// Helper to allocate string
static char* allocString(const std::string& s) {
    char* result = (char*)malloc(s.size() + 1);
    if (result) {
        memcpy(result, s.c_str(), s.size() + 1);
    }
    return result;
}

static void initWinsock() {
#ifdef _WIN32
    static bool initialized = false;
    if (!initialized) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        initialized = true;
    }
#endif
}

extern "C" {

// ============================================================================
// IP Utilities
// ============================================================================

int nova_net_isIP(const char* input) {
    if (!input) return 0;

    struct in_addr addr4;
    struct in6_addr addr6;

    if (inet_pton(AF_INET, input, &addr4) == 1) return 4;
    if (inet_pton(AF_INET6, input, &addr6) == 1) return 6;
    return 0;
}

int nova_net_isIPv4(const char* input) {
    return nova_net_isIP(input) == 4 ? 1 : 0;
}

int nova_net_isIPv6(const char* input) {
    return nova_net_isIP(input) == 6 ? 1 : 0;
}

// ============================================================================
// BlockList class
// ============================================================================

void* nova_net_BlockList_new() {
    return new NovaBlockList();
}

void nova_net_BlockList_free(void* blocklist) {
    NovaBlockList* bl = (NovaBlockList*)blocklist;
    if (bl) delete bl;
}

void nova_net_BlockList_addAddress(void* blocklist, const char* address, const char* family) {
    NovaBlockList* bl = (NovaBlockList*)blocklist;
    if (bl && address) {
        std::string rule = std::string(address) + "/" + (family ? family : "ipv4");
        bl->rules.insert(rule);
    }
}

void nova_net_BlockList_addRange(void* blocklist, const char* start, const char* end, const char* family) {
    NovaBlockList* bl = (NovaBlockList*)blocklist;
    if (bl && start && end) {
        std::string rule = std::string(start) + "-" + end + "/" + (family ? family : "ipv4");
        bl->rules.insert(rule);
    }
}

void nova_net_BlockList_addSubnet(void* blocklist, const char* network, int prefix, const char* family) {
    NovaBlockList* bl = (NovaBlockList*)blocklist;
    if (bl && network) {
        char rule[256];
        snprintf(rule, sizeof(rule), "%s/%d/%s", network, prefix, family ? family : "ipv4");
        bl->rules.insert(rule);
    }
}

int nova_net_BlockList_check(void* blocklist, const char* address, const char* family) {
    NovaBlockList* bl = (NovaBlockList*)blocklist;
    if (!bl || !address) return 0;

    // Simplified check - just look for exact match
    std::string rule = std::string(address) + "/" + (family ? family : "ipv4");
    return bl->rules.find(rule) != bl->rules.end() ? 1 : 0;
}

char** nova_net_BlockList_rules(void* blocklist, int* count) {
    NovaBlockList* bl = (NovaBlockList*)blocklist;
    if (!bl || !count) return nullptr;

    *count = (int)bl->rules.size();
    if (*count == 0) return nullptr;

    char** result = (char**)malloc(sizeof(char*) * (*count));
    int i = 0;
    for (const std::string& rule : bl->rules) {
        result[i++] = allocString(rule);
    }
    return result;
}

// ============================================================================
// SocketAddress class
// ============================================================================

void* nova_net_SocketAddress_new(const char* address, int port, const char* family, int flowlabel) {
    NovaSocketAddress* sa = new NovaSocketAddress();
    sa->address = allocString(address ? address : "127.0.0.1");
    sa->family = allocString(family ? family : "ipv4");
    sa->port = port;
    sa->flowlabel = flowlabel;
    return sa;
}

void nova_net_SocketAddress_free(void* socketAddress) {
    NovaSocketAddress* sa = (NovaSocketAddress*)socketAddress;
    if (sa) {
        free(sa->address);
        free(sa->family);
        delete sa;
    }
}

char* nova_net_SocketAddress_address(void* socketAddress) {
    NovaSocketAddress* sa = (NovaSocketAddress*)socketAddress;
    return sa ? allocString(sa->address) : nullptr;
}

char* nova_net_SocketAddress_family(void* socketAddress) {
    NovaSocketAddress* sa = (NovaSocketAddress*)socketAddress;
    return sa ? allocString(sa->family) : nullptr;
}

int nova_net_SocketAddress_port(void* socketAddress) {
    NovaSocketAddress* sa = (NovaSocketAddress*)socketAddress;
    return sa ? sa->port : 0;
}

int nova_net_SocketAddress_flowlabel(void* socketAddress) {
    NovaSocketAddress* sa = (NovaSocketAddress*)socketAddress;
    return sa ? sa->flowlabel : 0;
}

// ============================================================================
// Socket class
// ============================================================================

void* nova_net_Socket_new() {
    initWinsock();

    NovaSocket* sock = new NovaSocket();
    sock->fd = -1;
    sock->remoteAddress = nullptr;
    sock->remoteFamily = nullptr;
    sock->remotePort = 0;
    sock->localAddress = nullptr;
    sock->localFamily = nullptr;
    sock->localPort = 0;
    sock->bytesRead = 0;
    sock->bytesWritten = 0;
    sock->connecting = false;
    sock->destroyed = false;
    sock->pending = true;
    sock->readable = true;
    sock->writable = true;
    sock->timeout = 0;
    sock->allowHalfOpen = false;
    return sock;
}

void nova_net_Socket_free(void* socket) {
    NovaSocket* sock = (NovaSocket*)socket;
    if (sock) {
        if (sock->fd >= 0) {
#ifdef _WIN32
            closesocket(sock->fd);
#else
            close(sock->fd);
#endif
        }
        free(sock->remoteAddress);
        free(sock->remoteFamily);
        free(sock->localAddress);
        free(sock->localFamily);
        delete sock;
    }
}

int nova_net_Socket_connect(void* socket, int port, const char* host) {
    NovaSocket* sock = (NovaSocket*)socket;
    if (!sock) return -1;

    sock->fd = (int)::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock->fd < 0) return -1;

    sock->connecting = true;
    sock->pending = true;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (host && *host) {
        inet_pton(AF_INET, host, &addr.sin_addr);
    } else {
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }

    int result = ::connect(sock->fd, (struct sockaddr*)&addr, sizeof(addr));
    if (result < 0) {
#ifdef _WIN32
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            closesocket(sock->fd);
            sock->fd = -1;
            return -1;
        }
#else
        if (errno != EINPROGRESS) {
            close(sock->fd);
            sock->fd = -1;
            return -1;
        }
#endif
    }

    sock->connecting = false;
    sock->pending = false;

    // Store remote info
    free(sock->remoteAddress);
    sock->remoteAddress = allocString(host ? host : "127.0.0.1");
    free(sock->remoteFamily);
    sock->remoteFamily = allocString("IPv4");
    sock->remotePort = port;

    // Get local info
    struct sockaddr_in localAddr;
    socklen_t localLen = sizeof(localAddr);
    if (getsockname(sock->fd, (struct sockaddr*)&localAddr, &localLen) == 0) {
        char localIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &localAddr.sin_addr, localIP, sizeof(localIP));
        free(sock->localAddress);
        sock->localAddress = allocString(localIP);
        free(sock->localFamily);
        sock->localFamily = allocString("IPv4");
        sock->localPort = ntohs(localAddr.sin_port);
    }

    return 0;
}

int nova_net_Socket_write(void* socket, const char* data, int length) {
    NovaSocket* sock = (NovaSocket*)socket;
    if (!sock || sock->fd < 0 || !data) return -1;

    int sent = send(sock->fd, data, length, 0);
    if (sent > 0) {
        sock->bytesWritten += sent;
    }
    return sent;
}

int nova_net_Socket_read(void* socket, char* buffer, int length) {
    NovaSocket* sock = (NovaSocket*)socket;
    if (!sock || sock->fd < 0 || !buffer) return -1;

    int received = recv(sock->fd, buffer, length, 0);
    if (received > 0) {
        sock->bytesRead += received;
    }
    return received;
}

void nova_net_Socket_end(void* socket, const char* data, int length) {
    NovaSocket* sock = (NovaSocket*)socket;
    if (!sock) return;

    if (data && length > 0 && sock->fd >= 0) {
        send(sock->fd, data, length, 0);
        sock->bytesWritten += length;
    }

    sock->writable = false;
#ifdef _WIN32
    shutdown(sock->fd, SD_SEND);
#else
    shutdown(sock->fd, SHUT_WR);
#endif
}

void nova_net_Socket_destroy(void* socket) {
    NovaSocket* sock = (NovaSocket*)socket;
    if (!sock) return;

    if (sock->fd >= 0) {
#ifdef _WIN32
        closesocket(sock->fd);
#else
        close(sock->fd);
#endif
        sock->fd = -1;
    }
    sock->destroyed = true;
    sock->readable = false;
    sock->writable = false;
}

void nova_net_Socket_pause(void* socket) {
    NovaSocket* sock = (NovaSocket*)socket;
    if (sock) sock->readable = false;
}

void nova_net_Socket_resume(void* socket) {
    NovaSocket* sock = (NovaSocket*)socket;
    if (sock) sock->readable = true;
}

void nova_net_Socket_setTimeout(void* socket, int timeout) {
    NovaSocket* sock = (NovaSocket*)socket;
    if (!sock) return;

    sock->timeout = timeout;
    if (sock->fd >= 0) {
#ifdef _WIN32
        setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
        setsockopt(sock->fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#else
        struct timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sock->fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
    }
}

void nova_net_Socket_setNoDelay(void* socket, int noDelay) {
    NovaSocket* sock = (NovaSocket*)socket;
    if (sock && sock->fd >= 0) {
#ifdef _WIN32
        setsockopt(sock->fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));
#else
        setsockopt(sock->fd, IPPROTO_TCP, TCP_NODELAY, &noDelay, sizeof(noDelay));
#endif
    }
}

void nova_net_Socket_setKeepAlive(void* socket, int enable, int initialDelay) {
    NovaSocket* sock = (NovaSocket*)socket;
    if (sock && sock->fd >= 0) {
#ifdef _WIN32
        setsockopt(sock->fd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&enable, sizeof(enable));
#else
        setsockopt(sock->fd, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable));
#endif
        (void)initialDelay;
    }
}

char* nova_net_Socket_remoteAddress(void* socket) {
    NovaSocket* sock = (NovaSocket*)socket;
    return (sock && sock->remoteAddress) ? allocString(sock->remoteAddress) : nullptr;
}

char* nova_net_Socket_remoteFamily(void* socket) {
    NovaSocket* sock = (NovaSocket*)socket;
    return (sock && sock->remoteFamily) ? allocString(sock->remoteFamily) : nullptr;
}

int nova_net_Socket_remotePort(void* socket) {
    NovaSocket* sock = (NovaSocket*)socket;
    return sock ? sock->remotePort : 0;
}

char* nova_net_Socket_localAddress(void* socket) {
    NovaSocket* sock = (NovaSocket*)socket;
    return (sock && sock->localAddress) ? allocString(sock->localAddress) : nullptr;
}

char* nova_net_Socket_localFamily(void* socket) {
    NovaSocket* sock = (NovaSocket*)socket;
    return (sock && sock->localFamily) ? allocString(sock->localFamily) : nullptr;
}

int nova_net_Socket_localPort(void* socket) {
    NovaSocket* sock = (NovaSocket*)socket;
    return sock ? sock->localPort : 0;
}

int64_t nova_net_Socket_bytesRead(void* socket) {
    NovaSocket* sock = (NovaSocket*)socket;
    return sock ? sock->bytesRead : 0;
}

int64_t nova_net_Socket_bytesWritten(void* socket) {
    NovaSocket* sock = (NovaSocket*)socket;
    return sock ? sock->bytesWritten : 0;
}

int nova_net_Socket_connecting(void* socket) {
    NovaSocket* sock = (NovaSocket*)socket;
    return (sock && sock->connecting) ? 1 : 0;
}

int nova_net_Socket_destroyed(void* socket) {
    NovaSocket* sock = (NovaSocket*)socket;
    return (sock && sock->destroyed) ? 1 : 0;
}

int nova_net_Socket_pending(void* socket) {
    NovaSocket* sock = (NovaSocket*)socket;
    return (sock && sock->pending) ? 1 : 0;
}

char* nova_net_Socket_readyState(void* socket) {
    NovaSocket* sock = (NovaSocket*)socket;
    if (!sock) return allocString("closed");

    if (sock->connecting) return allocString("opening");
    if (sock->readable && sock->writable) return allocString("open");
    if (sock->readable) return allocString("readOnly");
    if (sock->writable) return allocString("writeOnly");
    return allocString("closed");
}

void nova_net_Socket_ref(void* socket) {
    // Keep socket in event loop
    (void)socket;
}

void nova_net_Socket_unref(void* socket) {
    // Allow event loop to exit
    (void)socket;
}

void nova_net_Socket_resetAndDestroy(void* socket) {
    NovaSocket* sock = (NovaSocket*)socket;
    if (sock && sock->fd >= 0) {
        struct linger l = { 1, 0 };
#ifdef _WIN32
        setsockopt(sock->fd, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof(l));
#else
        setsockopt(sock->fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
#endif
        nova_net_Socket_destroy(socket);
    }
}

void nova_net_Socket_on(void* socket, const char* event, void* callback) {
    NovaSocket* sock = (NovaSocket*)socket;
    if (sock && event) {
        sock->eventHandlers[event] = callback;
    }
}

void* nova_net_Socket_address(void* socket) {
    NovaSocket* sock = (NovaSocket*)socket;
    if (!sock) return nullptr;

    return nova_net_SocketAddress_new(sock->localAddress, sock->localPort, sock->localFamily, 0);
}

int nova_net_Socket_bufferSize(void* socket) {
    (void)socket;
    return 0;  // Simplified
}

// ============================================================================
// Server class
// ============================================================================

void* nova_net_Server_new() {
    initWinsock();

    NovaServer* server = new NovaServer();
    server->fd = -1;
    server->listening = false;
    server->maxConnections = 0;
    server->connections = 0;
    server->address = nullptr;
    server->port = 0;
    server->family = nullptr;
    return server;
}

void nova_net_Server_free(void* serverPtr) {
    NovaServer* server = (NovaServer*)serverPtr;
    if (server) {
        if (server->fd >= 0) {
#ifdef _WIN32
            closesocket(server->fd);
#else
            close(server->fd);
#endif
        }
        free(server->address);
        free(server->family);
        for (NovaSocket* client : server->clients) {
            nova_net_Socket_free(client);
        }
        delete server;
    }
}

int nova_net_Server_listen(void* serverPtr, int port, const char* host, int backlog) {
    NovaServer* server = (NovaServer*)serverPtr;
    if (!server) return -1;

    server->fd = (int)::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server->fd < 0) return -1;

    int opt = 1;
#ifdef _WIN32
    setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
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

    if (bind(server->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
#ifdef _WIN32
        closesocket(server->fd);
#else
        close(server->fd);
#endif
        server->fd = -1;
        return -1;
    }

    if (listen(server->fd, backlog > 0 ? backlog : SOMAXCONN) < 0) {
#ifdef _WIN32
        closesocket(server->fd);
#else
        close(server->fd);
#endif
        server->fd = -1;
        return -1;
    }

    server->listening = true;
    server->port = port;
    free(server->address);
    server->address = allocString(host ? host : "0.0.0.0");
    free(server->family);
    server->family = allocString("IPv4");

    return 0;
}

void nova_net_Server_close(void* serverPtr) {
    NovaServer* server = (NovaServer*)serverPtr;
    if (server && server->fd >= 0) {
#ifdef _WIN32
        closesocket(server->fd);
#else
        close(server->fd);
#endif
        server->fd = -1;
        server->listening = false;
    }
}

void* nova_net_Server_address(void* serverPtr) {
    NovaServer* server = (NovaServer*)serverPtr;
    if (!server) return nullptr;

    return nova_net_SocketAddress_new(server->address, server->port, server->family, 0);
}

int nova_net_Server_getConnections(void* serverPtr) {
    NovaServer* server = (NovaServer*)serverPtr;
    return server ? server->connections : 0;
}

void nova_net_Server_ref(void* serverPtr) {
    (void)serverPtr;
}

void nova_net_Server_unref(void* serverPtr) {
    (void)serverPtr;
}

int nova_net_Server_maxConnections(void* serverPtr) {
    NovaServer* server = (NovaServer*)serverPtr;
    return server ? server->maxConnections : 0;
}

void nova_net_Server_setMaxConnections(void* serverPtr, int max) {
    NovaServer* server = (NovaServer*)serverPtr;
    if (server) server->maxConnections = max;
}

int nova_net_Server_listening(void* serverPtr) {
    NovaServer* server = (NovaServer*)serverPtr;
    return (server && server->listening) ? 1 : 0;
}

void nova_net_Server_on(void* serverPtr, const char* event, void* callback) {
    NovaServer* server = (NovaServer*)serverPtr;
    if (server && event) {
        server->eventHandlers[event] = callback;
    }
}

void* nova_net_Server_accept(void* serverPtr) {
    NovaServer* server = (NovaServer*)serverPtr;
    if (!server || server->fd < 0) return nullptr;

    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    int clientFd = (int)accept(server->fd, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientFd < 0) return nullptr;

    NovaSocket* client = (NovaSocket*)nova_net_Socket_new();
    client->fd = clientFd;
    client->pending = false;
    client->connecting = false;

    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
    client->remoteAddress = allocString(clientIP);
    client->remoteFamily = allocString("IPv4");
    client->remotePort = ntohs(clientAddr.sin_port);

    struct sockaddr_in localAddr;
    socklen_t localLen = sizeof(localAddr);
    if (getsockname(clientFd, (struct sockaddr*)&localAddr, &localLen) == 0) {
        char localIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &localAddr.sin_addr, localIP, sizeof(localIP));
        client->localAddress = allocString(localIP);
        client->localFamily = allocString("IPv4");
        client->localPort = ntohs(localAddr.sin_port);
    }

    server->clients.push_back(client);
    server->connections++;

    return client;
}

// ============================================================================
// Module functions
// ============================================================================

void* nova_net_createServer() {
    return nova_net_Server_new();
}

void* nova_net_createConnection(int port, const char* host) {
    NovaSocket* sock = (NovaSocket*)nova_net_Socket_new();
    if (nova_net_Socket_connect(sock, port, host) < 0) {
        nova_net_Socket_free(sock);
        return nullptr;
    }
    return sock;
}

void* nova_net_connect(int port, const char* host) {
    return nova_net_createConnection(port, host);
}

// ============================================================================
// Utility functions
// ============================================================================

int nova_net_getDefaultAutoSelectFamily() {
    return 0;  // false by default
}

void nova_net_setDefaultAutoSelectFamily(int value) {
    (void)value;
}

int nova_net_getDefaultAutoSelectFamilyAttemptTimeout() {
    return 250;  // milliseconds
}

void nova_net_setDefaultAutoSelectFamilyAttemptTimeout(int timeout) {
    (void)timeout;
}

// ============================================================================
// Cleanup
// ============================================================================

void nova_net_cleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}

} // extern "C"
