// Nova Builtin Dgram Module Implementation
// Provides Node.js-compatible UDP/Datagram socket API

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

extern "C" {

// ============================================================================
// Winsock initialization (Windows only)
// ============================================================================

[[maybe_unused]] static std::atomic<bool> wsaInitialized{false};

static void ensureWsaInitialized() {
#ifdef _WIN32
    if (!wsaInitialized.exchange(true)) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    }
#endif
}

// ============================================================================
// Socket Structure
// ============================================================================

struct NovaDgramSocket {
    SOCKET fd;
    int type;  // 4 = udp4, 6 = udp6
    bool bound;
    bool connected;
    bool closed;
    bool reuseAddr;
    bool reusePort;
    bool broadcast;
    bool multicastLoopback;
    int multicastTTL;
    int ttl;
    int recvBufferSize;
    int sendBufferSize;
    std::string boundAddress;
    int boundPort;
    std::string remoteAddr;
    int remotePort;
    std::mutex mutex;
    std::atomic<bool> receiving{false};
    std::thread* recvThread;
    void* onMessageCallback;
    void* onErrorCallback;
    void* onCloseCallback;
    void* onListeningCallback;
};

// ============================================================================
// Socket Creation
// ============================================================================

// Create UDP socket - type: "udp4" or "udp6"
void* nova_dgram_createSocket(const char* type) {
    ensureWsaInitialized();

    NovaDgramSocket* sock = new NovaDgramSocket();
    sock->type = (type && strcmp(type, "udp6") == 0) ? 6 : 4;
    sock->bound = false;
    sock->connected = false;
    sock->closed = false;
    sock->reuseAddr = false;
    sock->reusePort = false;
    sock->broadcast = false;
    sock->multicastLoopback = true;
    sock->multicastTTL = 1;
    sock->ttl = 64;
    sock->recvBufferSize = 65536;
    sock->sendBufferSize = 65536;
    sock->boundPort = 0;
    sock->remotePort = 0;
    sock->recvThread = nullptr;
    sock->onMessageCallback = nullptr;
    sock->onErrorCallback = nullptr;
    sock->onCloseCallback = nullptr;
    sock->onListeningCallback = nullptr;

    int family = (sock->type == 6) ? AF_INET6 : AF_INET;
    sock->fd = socket(family, SOCK_DGRAM, IPPROTO_UDP);

    if (sock->fd == INVALID_SOCKET) {
        delete sock;
        return nullptr;
    }

    return sock;
}

// Create socket with options
void* nova_dgram_createSocketWithOptions(const char* type, int reuseAddr, int ipv6Only) {
    void* sockPtr = nova_dgram_createSocket(type);
    if (!sockPtr) return nullptr;

    NovaDgramSocket* sock = (NovaDgramSocket*)sockPtr;

    if (reuseAddr) {
        int opt = 1;
        setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
        sock->reuseAddr = true;
    }

#ifdef _WIN32
    // Windows doesn't have SO_REUSEPORT
#else
    // SO_REUSEPORT on Unix
#endif

    if (sock->type == 6 && ipv6Only) {
        int opt = 1;
        setsockopt(sock->fd, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&opt, sizeof(opt));
    }

    return sock;
}

// ============================================================================
// Socket Binding
// ============================================================================

// Bind socket to port and address
int nova_dgram_bind(void* socketPtr, int port, const char* address) {
    if (!socketPtr) return -1;
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;
    std::lock_guard<std::mutex> lock(sock->mutex);

    if (sock->closed || sock->bound) return -1;

    if (sock->type == 4) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        if (address && strlen(address) > 0) {
            inet_pton(AF_INET, address, &addr.sin_addr);
        } else {
            addr.sin_addr.s_addr = INADDR_ANY;
        }

        if (bind(sock->fd, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            return -1;
        }

        // Get actual bound port if port was 0
        socklen_t len = sizeof(addr);
        getsockname(sock->fd, (struct sockaddr*)&addr, &len);
        sock->boundPort = ntohs(addr.sin_port);
        sock->boundAddress = address ? address : "0.0.0.0";
    } else {
        struct sockaddr_in6 addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin6_family = AF_INET6;
        addr.sin6_port = htons(port);

        if (address && strlen(address) > 0) {
            inet_pton(AF_INET6, address, &addr.sin6_addr);
        } else {
            addr.sin6_addr = in6addr_any;
        }

        if (bind(sock->fd, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            return -1;
        }

        socklen_t len = sizeof(addr);
        getsockname(sock->fd, (struct sockaddr*)&addr, &len);
        sock->boundPort = ntohs(addr.sin6_port);
        sock->boundAddress = address ? address : "::";
    }

    sock->bound = true;
    return 0;
}

// Bind with exclusive flag
int nova_dgram_bindExclusive(void* socketPtr, int port, const char* address, int exclusive) {
    if (!socketPtr) return -1;
    [[maybe_unused]] NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;

#ifdef _WIN32
    if (exclusive) {
        int opt = 1;
        setsockopt(sock->fd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char*)&opt, sizeof(opt));
    }
#else
    (void)exclusive;
#endif

    return nova_dgram_bind(socketPtr, port, address);
}

// ============================================================================
// Socket Connection (UDP connect)
// ============================================================================

// Connect to remote address (for default destination)
int nova_dgram_connect(void* socketPtr, int port, const char* address) {
    if (!socketPtr || !address) return -1;
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;
    std::lock_guard<std::mutex> lock(sock->mutex);

    if (sock->closed) return -1;

    if (sock->type == 4) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, address, &addr.sin_addr);

        if (connect(sock->fd, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            return -1;
        }
    } else {
        struct sockaddr_in6 addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin6_family = AF_INET6;
        addr.sin6_port = htons(port);
        inet_pton(AF_INET6, address, &addr.sin6_addr);

        if (connect(sock->fd, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            return -1;
        }
    }

    sock->connected = true;
    sock->remoteAddr = address;
    sock->remotePort = port;
    return 0;
}

// Disconnect (remove default destination)
int nova_dgram_disconnect(void* socketPtr) {
    if (!socketPtr) return -1;
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;
    std::lock_guard<std::mutex> lock(sock->mutex);

    if (sock->closed || !sock->connected) return -1;

    // Connect to AF_UNSPEC to disconnect
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_UNSPEC;
    connect(sock->fd, (struct sockaddr*)&addr, sizeof(addr));

    sock->connected = false;
    sock->remoteAddr.clear();
    sock->remotePort = 0;
    return 0;
}

// ============================================================================
// Send/Receive
// ============================================================================

// Send data to specific address
int nova_dgram_send(void* socketPtr, const char* data, int length, int port, const char* address) {
    if (!socketPtr || !data) return -1;
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;
    std::lock_guard<std::mutex> lock(sock->mutex);

    if (sock->closed) return -1;

    int sent;
    if (address && port > 0) {
        if (sock->type == 4) {
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            inet_pton(AF_INET, address, &addr.sin_addr);
            sent = sendto(sock->fd, data, length, 0, (struct sockaddr*)&addr, sizeof(addr));
        } else {
            struct sockaddr_in6 addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin6_family = AF_INET6;
            addr.sin6_port = htons(port);
            inet_pton(AF_INET6, address, &addr.sin6_addr);
            sent = sendto(sock->fd, data, length, 0, (struct sockaddr*)&addr, sizeof(addr));
        }
    } else if (sock->connected) {
        sent = send(sock->fd, data, length, 0);
    } else {
        return -1;
    }

    return sent;
}

// Send with offset and length
int nova_dgram_sendOffset(void* socketPtr, const char* data, int offset, int length, int port, const char* address) {
    return nova_dgram_send(socketPtr, data + offset, length, port, address);
}

// Receive data (blocking)
int nova_dgram_recv(void* socketPtr, char* buffer, int bufferSize, char* fromAddr, int* fromPort) {
    if (!socketPtr || !buffer) return -1;
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;

    if (sock->closed) return -1;

    struct sockaddr_storage srcAddr;
    socklen_t addrLen = sizeof(srcAddr);

    int received = recvfrom(sock->fd, buffer, bufferSize, 0, (struct sockaddr*)&srcAddr, &addrLen);

    if (received > 0 && fromAddr && fromPort) {
        if (srcAddr.ss_family == AF_INET) {
            struct sockaddr_in* addr4 = (struct sockaddr_in*)&srcAddr;
            inet_ntop(AF_INET, &addr4->sin_addr, fromAddr, 46);
            *fromPort = ntohs(addr4->sin_port);
        } else {
            struct sockaddr_in6* addr6 = (struct sockaddr_in6*)&srcAddr;
            inet_ntop(AF_INET6, &addr6->sin6_addr, fromAddr, 46);
            *fromPort = ntohs(addr6->sin6_port);
        }
    }

    return received;
}

// ============================================================================
// Close
// ============================================================================

// Close socket
void nova_dgram_close(void* socketPtr) {
    if (!socketPtr) return;
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;

    sock->receiving = false;

    {
        std::lock_guard<std::mutex> lock(sock->mutex);
        if (sock->closed) return;
        sock->closed = true;
        closesocket(sock->fd);
    }

    if (sock->recvThread && sock->recvThread->joinable()) {
        sock->recvThread->join();
        delete sock->recvThread;
    }
}

// Free socket memory
void nova_dgram_free(void* socketPtr) {
    if (!socketPtr) return;
    nova_dgram_close(socketPtr);
    delete (NovaDgramSocket*)socketPtr;
}

// ============================================================================
// Address Information
// ============================================================================

// Get local address info
const char* nova_dgram_address(void* socketPtr) {
    if (!socketPtr) return "";
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;
    return sock->boundAddress.c_str();
}

int nova_dgram_port(void* socketPtr) {
    if (!socketPtr) return 0;
    return ((NovaDgramSocket*)socketPtr)->boundPort;
}

const char* nova_dgram_family(void* socketPtr) {
    if (!socketPtr) return "udp4";
    return ((NovaDgramSocket*)socketPtr)->type == 6 ? "udp6" : "udp4";
}

// Get remote address (if connected)
const char* nova_dgram_remoteAddress(void* socketPtr) {
    if (!socketPtr) return "";
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;
    return sock->remoteAddr.c_str();
}

int nova_dgram_remotePort(void* socketPtr) {
    if (!socketPtr) return 0;
    return ((NovaDgramSocket*)socketPtr)->remotePort;
}

// ============================================================================
// Socket Options
// ============================================================================

// Set broadcast
int nova_dgram_setBroadcast(void* socketPtr, int flag) {
    if (!socketPtr) return -1;
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;
    int opt = flag ? 1 : 0;
    int result = setsockopt(sock->fd, SOL_SOCKET, SO_BROADCAST, (const char*)&opt, sizeof(opt));
    if (result == 0) sock->broadcast = (flag != 0);
    return result;
}

// Set TTL
int nova_dgram_setTTL(void* socketPtr, int ttl) {
    if (!socketPtr) return -1;
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;
    int result;
    if (sock->type == 4) {
        result = setsockopt(sock->fd, IPPROTO_IP, IP_TTL, (const char*)&ttl, sizeof(ttl));
    } else {
        result = setsockopt(sock->fd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, (const char*)&ttl, sizeof(ttl));
    }
    if (result == 0) sock->ttl = ttl;
    return result;
}

// Set multicast TTL
int nova_dgram_setMulticastTTL(void* socketPtr, int ttl) {
    if (!socketPtr) return -1;
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;
    int result;
    if (sock->type == 4) {
        unsigned char mttl = (unsigned char)ttl;
        result = setsockopt(sock->fd, IPPROTO_IP, IP_MULTICAST_TTL, (const char*)&mttl, sizeof(mttl));
    } else {
        result = setsockopt(sock->fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (const char*)&ttl, sizeof(ttl));
    }
    if (result == 0) sock->multicastTTL = ttl;
    return result;
}

// Set multicast loopback
int nova_dgram_setMulticastLoopback(void* socketPtr, int flag) {
    if (!socketPtr) return -1;
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;
    int result;
    if (sock->type == 4) {
        unsigned char loop = flag ? 1 : 0;
        result = setsockopt(sock->fd, IPPROTO_IP, IP_MULTICAST_LOOP, (const char*)&loop, sizeof(loop));
    } else {
        int loop = flag ? 1 : 0;
        result = setsockopt(sock->fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (const char*)&loop, sizeof(loop));
    }
    if (result == 0) sock->multicastLoopback = (flag != 0);
    return result;
}

// Set multicast interface
int nova_dgram_setMulticastInterface(void* socketPtr, const char* interfaceAddr) {
    if (!socketPtr || !interfaceAddr) return -1;
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;

    if (sock->type == 4) {
        struct in_addr addr;
        inet_pton(AF_INET, interfaceAddr, &addr);
        return setsockopt(sock->fd, IPPROTO_IP, IP_MULTICAST_IF, (const char*)&addr, sizeof(addr));
    } else {
        // For IPv6, use interface index
        unsigned int ifindex = 0;  // 0 = default
        return setsockopt(sock->fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, (const char*)&ifindex, sizeof(ifindex));
    }
}

// ============================================================================
// Multicast Membership
// ============================================================================

// Add multicast membership
int nova_dgram_addMembership(void* socketPtr, const char* multicastAddr, const char* interfaceAddr) {
    if (!socketPtr || !multicastAddr) return -1;
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;

    if (sock->type == 4) {
        struct ip_mreq mreq;
        inet_pton(AF_INET, multicastAddr, &mreq.imr_multiaddr);
        if (interfaceAddr && strlen(interfaceAddr) > 0) {
            inet_pton(AF_INET, interfaceAddr, &mreq.imr_interface);
        } else {
            mreq.imr_interface.s_addr = INADDR_ANY;
        }
        return setsockopt(sock->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&mreq, sizeof(mreq));
    } else {
        struct ipv6_mreq mreq;
        inet_pton(AF_INET6, multicastAddr, &mreq.ipv6mr_multiaddr);
        mreq.ipv6mr_interface = 0;
        return setsockopt(sock->fd, IPPROTO_IPV6, IPV6_JOIN_GROUP, (const char*)&mreq, sizeof(mreq));
    }
}

// Drop multicast membership
int nova_dgram_dropMembership(void* socketPtr, const char* multicastAddr, const char* interfaceAddr) {
    if (!socketPtr || !multicastAddr) return -1;
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;

    if (sock->type == 4) {
        struct ip_mreq mreq;
        inet_pton(AF_INET, multicastAddr, &mreq.imr_multiaddr);
        if (interfaceAddr && strlen(interfaceAddr) > 0) {
            inet_pton(AF_INET, interfaceAddr, &mreq.imr_interface);
        } else {
            mreq.imr_interface.s_addr = INADDR_ANY;
        }
        return setsockopt(sock->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (const char*)&mreq, sizeof(mreq));
    } else {
        struct ipv6_mreq mreq;
        inet_pton(AF_INET6, multicastAddr, &mreq.ipv6mr_multiaddr);
        mreq.ipv6mr_interface = 0;
        return setsockopt(sock->fd, IPPROTO_IPV6, IPV6_LEAVE_GROUP, (const char*)&mreq, sizeof(mreq));
    }
}

// Add source-specific membership
int nova_dgram_addSourceSpecificMembership(void* socketPtr, const char* sourceAddr, const char* groupAddr, const char* interfaceAddr) {
    if (!socketPtr || !sourceAddr || !groupAddr) return -1;
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;

#ifdef IP_ADD_SOURCE_MEMBERSHIP
    if (sock->type == 4) {
        struct ip_mreq_source mreq;
        inet_pton(AF_INET, groupAddr, &mreq.imr_multiaddr);
        inet_pton(AF_INET, sourceAddr, &mreq.imr_sourceaddr);
        if (interfaceAddr && strlen(interfaceAddr) > 0) {
            inet_pton(AF_INET, interfaceAddr, &mreq.imr_interface);
        } else {
            mreq.imr_interface.s_addr = INADDR_ANY;
        }
        return setsockopt(sock->fd, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, (const char*)&mreq, sizeof(mreq));
    }
#endif
    return -1;
}

// Drop source-specific membership
int nova_dgram_dropSourceSpecificMembership(void* socketPtr, const char* sourceAddr, const char* groupAddr, const char* interfaceAddr) {
    if (!socketPtr || !sourceAddr || !groupAddr) return -1;
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;

#ifdef IP_DROP_SOURCE_MEMBERSHIP
    if (sock->type == 4) {
        struct ip_mreq_source mreq;
        inet_pton(AF_INET, groupAddr, &mreq.imr_multiaddr);
        inet_pton(AF_INET, sourceAddr, &mreq.imr_sourceaddr);
        if (interfaceAddr && strlen(interfaceAddr) > 0) {
            inet_pton(AF_INET, interfaceAddr, &mreq.imr_interface);
        } else {
            mreq.imr_interface.s_addr = INADDR_ANY;
        }
        return setsockopt(sock->fd, IPPROTO_IP, IP_DROP_SOURCE_MEMBERSHIP, (const char*)&mreq, sizeof(mreq));
    }
#endif
    return -1;
}

// ============================================================================
// Buffer Sizes
// ============================================================================

// Get receive buffer size
int nova_dgram_getRecvBufferSize(void* socketPtr) {
    if (!socketPtr) return 0;
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;
    int size = 0;
    socklen_t len = sizeof(size);
    getsockopt(sock->fd, SOL_SOCKET, SO_RCVBUF, (char*)&size, &len);
    return size;
}

// Set receive buffer size
int nova_dgram_setRecvBufferSize(void* socketPtr, int size) {
    if (!socketPtr) return -1;
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;
    int result = setsockopt(sock->fd, SOL_SOCKET, SO_RCVBUF, (const char*)&size, sizeof(size));
    if (result == 0) sock->recvBufferSize = size;
    return result;
}

// Get send buffer size
int nova_dgram_getSendBufferSize(void* socketPtr) {
    if (!socketPtr) return 0;
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;
    int size = 0;
    socklen_t len = sizeof(size);
    getsockopt(sock->fd, SOL_SOCKET, SO_SNDBUF, (char*)&size, &len);
    return size;
}

// Set send buffer size
int nova_dgram_setSendBufferSize(void* socketPtr, int size) {
    if (!socketPtr) return -1;
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;
    int result = setsockopt(sock->fd, SOL_SOCKET, SO_SNDBUF, (const char*)&size, sizeof(size));
    if (result == 0) sock->sendBufferSize = size;
    return result;
}

// Get send queue size (platform-specific)
int nova_dgram_getSendQueueSize(void* socketPtr) {
    if (!socketPtr) return 0;
#ifdef _WIN32
    return 0;  // Not directly available on Windows
#else
    [[maybe_unused]] NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;
    int size = 0;
#ifdef SIOCOUTQ
    ioctl(sock->fd, SIOCOUTQ, &size);
#endif
    return size;
#endif
}

// Get send queue count
int nova_dgram_getSendQueueCount(void* socketPtr) {
    (void)socketPtr;
    return 0;  // Not directly available
}

// ============================================================================
// Reference Counting (event loop integration)
// ============================================================================

void nova_dgram_ref(void* socketPtr) {
    (void)socketPtr;
    // Placeholder for event loop reference
}

void nova_dgram_unref(void* socketPtr) {
    (void)socketPtr;
    // Placeholder for event loop unreference
}

// ============================================================================
// Socket State
// ============================================================================

int nova_dgram_isBound(void* socketPtr) {
    if (!socketPtr) return 0;
    return ((NovaDgramSocket*)socketPtr)->bound ? 1 : 0;
}

int nova_dgram_isConnected(void* socketPtr) {
    if (!socketPtr) return 0;
    return ((NovaDgramSocket*)socketPtr)->connected ? 1 : 0;
}

int nova_dgram_isClosed(void* socketPtr) {
    if (!socketPtr) return 1;
    return ((NovaDgramSocket*)socketPtr)->closed ? 1 : 0;
}

int nova_dgram_fd(void* socketPtr) {
    if (!socketPtr) return -1;
    return (int)((NovaDgramSocket*)socketPtr)->fd;
}

// ============================================================================
// Event Callbacks
// ============================================================================

void nova_dgram_onMessage(void* socketPtr, void* callback) {
    if (!socketPtr) return;
    ((NovaDgramSocket*)socketPtr)->onMessageCallback = callback;
}

void nova_dgram_onError(void* socketPtr, void* callback) {
    if (!socketPtr) return;
    ((NovaDgramSocket*)socketPtr)->onErrorCallback = callback;
}

void nova_dgram_onClose(void* socketPtr, void* callback) {
    if (!socketPtr) return;
    ((NovaDgramSocket*)socketPtr)->onCloseCallback = callback;
}

void nova_dgram_onListening(void* socketPtr, void* callback) {
    if (!socketPtr) return;
    ((NovaDgramSocket*)socketPtr)->onListeningCallback = callback;
}

// ============================================================================
// Non-blocking receive with callback
// ============================================================================

typedef void (*DgramMessageCallback)(const char* data, int length, const char* address, int port);

void nova_dgram_startReceiving(void* socketPtr, DgramMessageCallback callback) {
    if (!socketPtr || !callback) return;
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;

    if (sock->receiving.exchange(true)) return;  // Already receiving

    sock->recvThread = new std::thread([sock, callback]() {
        char buffer[65536];
        char fromAddr[46];
        int fromPort;

        while (sock->receiving && !sock->closed) {
            fromPort = 0;
            int received = nova_dgram_recv(sock, buffer, sizeof(buffer) - 1, fromAddr, &fromPort);
            if (received > 0) {
                buffer[received] = '\0';
                callback(buffer, received, fromAddr, fromPort);
            }
        }
    });
}

void nova_dgram_stopReceiving(void* socketPtr) {
    if (!socketPtr) return;
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;
    sock->receiving = false;
}

// ============================================================================
// Utility Functions
// ============================================================================

// Set socket to non-blocking mode
int nova_dgram_setNonBlocking(void* socketPtr, int nonBlocking) {
    if (!socketPtr) return -1;
    NovaDgramSocket* sock = (NovaDgramSocket*)socketPtr;

#ifdef _WIN32
    u_long mode = nonBlocking ? 1 : 0;
    return ioctlsocket(sock->fd, FIONBIO, &mode);
#else
    int flags = fcntl(sock->fd, F_GETFL, 0);
    if (flags == -1) return -1;
    if (nonBlocking) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    return fcntl(sock->fd, F_SETFL, flags);
#endif
}

// Get last error
int nova_dgram_getLastError() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

// Error string
const char* nova_dgram_errorString(int error) {
#ifdef _WIN32
    static char buffer[256];
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, buffer, sizeof(buffer), NULL);
    return buffer;
#else
    return strerror(error);
#endif
}

// Cleanup (call at program exit)
void nova_dgram_cleanup() {
#ifdef _WIN32
    if (wsaInitialized) {
        WSACleanup();
        wsaInitialized = false;
    }
#endif
}

} // extern "C"
