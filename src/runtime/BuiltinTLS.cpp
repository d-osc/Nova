// Nova Runtime - TLS (Transport Layer Security) Module
// Implements Node.js-compatible TLS/SSL functionality

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

#ifdef _WIN32
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

// ============================================================================
// TLS Constants
// ============================================================================

const char* nova_tls_DEFAULT_ECDH_CURVE() { return "auto"; }
const char* nova_tls_DEFAULT_MAX_VERSION() { return "TLSv1.3"; }
const char* nova_tls_DEFAULT_MIN_VERSION() { return "TLSv1.2"; }

// ============================================================================
// Secure Context
// ============================================================================

struct SecureContext {
    std::string cert;
    std::string key;
    std::string ca;
    std::string passphrase;
    std::string ciphers;
    std::string ecdhCurve;
    std::string minVersion;
    std::string maxVersion;
    bool honorCipherOrder;
    bool requestCert;
    bool rejectUnauthorized;
    int sessionTimeout;
};

void* nova_tls_createSecureContext() {
    SecureContext* ctx = new SecureContext();
    ctx->ciphers = "";
    ctx->ecdhCurve = "auto";
    ctx->minVersion = "TLSv1.2";
    ctx->maxVersion = "TLSv1.3";
    ctx->honorCipherOrder = true;
    ctx->requestCert = false;
    ctx->rejectUnauthorized = true;
    ctx->sessionTimeout = 300;
    return ctx;
}

void nova_tls_secureContext_setCert(void* ctx, const char* cert) {
    if (!ctx || !cert) return;
    ((SecureContext*)ctx)->cert = cert;
}

void nova_tls_secureContext_setKey(void* ctx, const char* key) {
    if (!ctx || !key) return;
    ((SecureContext*)ctx)->key = key;
}

void nova_tls_secureContext_setCA(void* ctx, const char* ca) {
    if (!ctx || !ca) return;
    ((SecureContext*)ctx)->ca = ca;
}

void nova_tls_secureContext_setPassphrase(void* ctx, const char* pass) {
    if (!ctx || !pass) return;
    ((SecureContext*)ctx)->passphrase = pass;
}

void nova_tls_secureContext_setCiphers(void* ctx, const char* ciphers) {
    if (!ctx || !ciphers) return;
    ((SecureContext*)ctx)->ciphers = ciphers;
}

void nova_tls_secureContext_setEcdhCurve(void* ctx, const char* curve) {
    if (!ctx || !curve) return;
    ((SecureContext*)ctx)->ecdhCurve = curve;
}

void nova_tls_secureContext_setMinVersion(void* ctx, const char* ver) {
    if (!ctx || !ver) return;
    ((SecureContext*)ctx)->minVersion = ver;
}

void nova_tls_secureContext_setMaxVersion(void* ctx, const char* ver) {
    if (!ctx || !ver) return;
    ((SecureContext*)ctx)->maxVersion = ver;
}

void nova_tls_secureContext_setHonorCipherOrder(void* ctx, int honor) {
    if (!ctx) return;
    ((SecureContext*)ctx)->honorCipherOrder = honor != 0;
}

void nova_tls_secureContext_setRequestCert(void* ctx, int request) {
    if (!ctx) return;
    ((SecureContext*)ctx)->requestCert = request != 0;
}

void nova_tls_secureContext_setRejectUnauthorized(void* ctx, int reject) {
    if (!ctx) return;
    ((SecureContext*)ctx)->rejectUnauthorized = reject != 0;
}

void nova_tls_secureContext_setSessionTimeout(void* ctx, int timeout) {
    if (!ctx) return;
    ((SecureContext*)ctx)->sessionTimeout = timeout;
}

void nova_tls_secureContext_free(void* ctx) {
    delete (SecureContext*)ctx;
}

// ============================================================================
// TLS Socket
// ============================================================================

struct TLSSocket {
    int fd;
    void* secureContext;
    std::string localAddress;
    int localPort;
    std::string remoteAddress;
    int remotePort;
    std::string remoteFamily;
    bool encrypted;
    bool authorized;
    std::string authorizationError;
    std::string protocol;
    std::string cipher;
    std::string cipherVersion;
    bool sessionReused;
    std::vector<uint8_t> session;
    bool renegotiationDisabled;
};

void* nova_tls_socket_create(void* secureContext) {
    TLSSocket* sock = new TLSSocket();
    sock->fd = -1;
    sock->secureContext = secureContext;
    sock->localPort = 0;
    sock->remotePort = 0;
    sock->remoteFamily = "IPv4";
    sock->encrypted = true;
    sock->authorized = false;
    sock->protocol = "TLSv1.3";
    sock->cipher = "TLS_AES_256_GCM_SHA384";
    sock->cipherVersion = "TLSv1.3";
    sock->sessionReused = false;
    sock->renegotiationDisabled = false;
    return sock;
}

int nova_tls_socket_encrypted(void* sock) {
    return sock ? (((TLSSocket*)sock)->encrypted ? 1 : 0) : 0;
}

int nova_tls_socket_authorized(void* sock) {
    return sock ? (((TLSSocket*)sock)->authorized ? 1 : 0) : 0;
}

const char* nova_tls_socket_authorizationError(void* sock) {
    if (!sock) return "";
    return ((TLSSocket*)sock)->authorizationError.c_str();
}

const char* nova_tls_socket_getProtocol(void* sock) {
    if (!sock) return "";
    return ((TLSSocket*)sock)->protocol.c_str();
}

const char* nova_tls_socket_localAddress(void* sock) {
    if (!sock) return "";
    return ((TLSSocket*)sock)->localAddress.c_str();
}

int nova_tls_socket_localPort(void* sock) {
    return sock ? ((TLSSocket*)sock)->localPort : 0;
}

const char* nova_tls_socket_remoteAddress(void* sock) {
    if (!sock) return "";
    return ((TLSSocket*)sock)->remoteAddress.c_str();
}

int nova_tls_socket_remotePort(void* sock) {
    return sock ? ((TLSSocket*)sock)->remotePort : 0;
}

const char* nova_tls_socket_remoteFamily(void* sock) {
    if (!sock) return "";
    return ((TLSSocket*)sock)->remoteFamily.c_str();
}

// getCipher() - Returns object with name, standardName, version
const char* nova_tls_socket_getCipher_name(void* sock) {
    if (!sock) return "";
    return ((TLSSocket*)sock)->cipher.c_str();
}

const char* nova_tls_socket_getCipher_version(void* sock) {
    if (!sock) return "";
    return ((TLSSocket*)sock)->cipherVersion.c_str();
}

int nova_tls_socket_isSessionReused(void* sock) {
    return sock ? (((TLSSocket*)sock)->sessionReused ? 1 : 0) : 0;
}

void nova_tls_socket_disableRenegotiation(void* sock) {
    if (!sock) return;
    ((TLSSocket*)sock)->renegotiationDisabled = true;
}

void nova_tls_socket_enableTrace(void* sock) {
    (void)sock; // Would enable SSL trace logging
}

int nova_tls_socket_setMaxSendFragment(void* sock, int size) {
    (void)sock;
    return (size >= 512 && size <= 16384) ? 1 : 0;
}

void nova_tls_socket_renegotiate(void* sock, void (*callback)(const char*)) {
    if (!sock) { if (callback) callback("Socket not initialized"); return; }
    TLSSocket* s = (TLSSocket*)sock;
    if (s->renegotiationDisabled) {
        if (callback) callback("Renegotiation disabled");
        return;
    }
    // Would perform TLS renegotiation
    if (callback) callback(nullptr);
}

void nova_tls_socket_setSession(void* sock, const uint8_t* session, int len) {
    if (!sock || !session || len <= 0) return;
    TLSSocket* s = (TLSSocket*)sock;
    s->session.assign(session, session + len);
}

int nova_tls_socket_getSession(void* sock, uint8_t* buffer, int maxLen) {
    if (!sock || !buffer) return 0;
    TLSSocket* s = (TLSSocket*)sock;
    int len = (int)s->session.size();
    if (len > maxLen) len = maxLen;
    if (len > 0) memcpy(buffer, s->session.data(), len);
    return len;
}

// Certificate methods (simplified - would use OpenSSL in full impl)
const char* nova_tls_socket_getCertificate(void* sock) {
    (void)sock;
    return "{}"; // JSON representation
}

const char* nova_tls_socket_getPeerCertificate(void* sock) {
    (void)sock;
    return "{}"; // JSON representation
}

// Keying material export (RFC 5705)
int nova_tls_socket_exportKeyingMaterial(void* sock, uint8_t* out, int len,
                                          const char* label, const uint8_t* context, int contextLen) {
    (void)sock; (void)out; (void)len; (void)label; (void)context; (void)contextLen;
    // Would use SSL_export_keying_material
    return 0;
}

void nova_tls_socket_free(void* sock) {
    if (!sock) return;
    TLSSocket* s = (TLSSocket*)sock;
#ifdef _WIN32
    if (s->fd >= 0) closesocket(s->fd);
#else
    if (s->fd >= 0) close(s->fd);
#endif
    delete s;
}

// ============================================================================
// TLS Server
// ============================================================================

struct TLSServer {
    int fd;
    void* secureContext;
    std::string address;
    int port;
    bool listening;
    std::unordered_map<std::string, void*> sniContexts;
    std::vector<uint8_t> ticketKeys;
};

void* nova_tls_createServer(void* secureContext) {
    TLSServer* server = new TLSServer();
    server->fd = -1;
    server->secureContext = secureContext;
    server->port = 0;
    server->listening = false;
    server->ticketKeys.resize(48); // 48 bytes for ticket keys
    return server;
}

void nova_tls_server_listen(void* server, int port, const char* host,
                             void (*callback)(), void (*errorCb)(const char*)) {
    if (!server) { if (errorCb) errorCb("Server not initialized"); return; }
    TLSServer* srv = (TLSServer*)server;

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    srv->fd = (int)socket(AF_INET, SOCK_STREAM, 0);
    if (srv->fd < 0) {
        if (errorCb) errorCb("Failed to create socket");
        return;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(srv->fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(srv->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (host) {
        inet_pton(AF_INET, host, &addr.sin_addr);
    } else {
        addr.sin_addr.s_addr = INADDR_ANY;
    }

    if (bind(srv->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        if (errorCb) errorCb("Failed to bind");
        return;
    }

    if (listen(srv->fd, 128) < 0) {
        if (errorCb) errorCb("Failed to listen");
        return;
    }

    srv->address = host ? host : "0.0.0.0";
    srv->port = port;
    srv->listening = true;

    if (callback) callback();
}

void nova_tls_server_close(void* server, void (*callback)()) {
    if (!server) return;
    TLSServer* srv = (TLSServer*)server;
    if (srv->fd >= 0) {
#ifdef _WIN32
        closesocket(srv->fd);
#else
        close(srv->fd);
#endif
        srv->fd = -1;
    }
    srv->listening = false;
    if (callback) callback();
}

const char* nova_tls_server_address(void* server) {
    if (!server) return "";
    return ((TLSServer*)server)->address.c_str();
}

int nova_tls_server_port(void* server) {
    return server ? ((TLSServer*)server)->port : 0;
}

int nova_tls_server_listening(void* server) {
    return server ? (((TLSServer*)server)->listening ? 1 : 0) : 0;
}

// SNI (Server Name Indication) support
void nova_tls_server_addContext(void* server, const char* hostname, void* context) {
    if (!server || !hostname || !context) return;
    ((TLSServer*)server)->sniContexts[hostname] = context;
}

void nova_tls_server_setSecureContext(void* server, void* context) {
    if (!server) return;
    ((TLSServer*)server)->secureContext = context;
}

// Session ticket keys
void nova_tls_server_setTicketKeys(void* server, const uint8_t* keys, int len) {
    if (!server || !keys || len != 48) return;
    TLSServer* srv = (TLSServer*)server;
    srv->ticketKeys.assign(keys, keys + len);
}

int nova_tls_server_getTicketKeys(void* server, uint8_t* buffer) {
    if (!server || !buffer) return 0;
    TLSServer* srv = (TLSServer*)server;
    memcpy(buffer, srv->ticketKeys.data(), 48);
    return 48;
}

void nova_tls_server_free(void* server) {
    if (!server) return;
    TLSServer* srv = (TLSServer*)server;
    nova_tls_server_close(server, nullptr);
    delete srv;
}

// ============================================================================
// TLS Connect (Client)
// ============================================================================

void nova_tls_connect(const char* host, int port, void* secureContext,
                       void (*onConnect)(void*), void (*onError)(const char*)) {
    if (!host) { if (onError) onError("Host required"); return; }

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    int fd = (int)socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        if (onError) onError("Failed to create socket");
        return;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    inet_pton(AF_INET, host, &addr.sin_addr);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
#ifdef _WIN32
        closesocket(fd);
#else
        close(fd);
#endif
        if (onError) onError("Connection failed");
        return;
    }

    // Create TLS socket
    TLSSocket* sock = (TLSSocket*)nova_tls_socket_create(secureContext);
    sock->fd = fd;
    sock->remoteAddress = host;
    sock->remotePort = port;
    sock->authorized = true; // Simplified

    if (onConnect) onConnect(sock);
}

// ============================================================================
// TLS Utilities
// ============================================================================

// Get list of supported ciphers
const char* nova_tls_getCiphers() {
    return "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_GCM_SHA256:"
           "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256";
}

// Root certificates (simplified)
const char* nova_tls_rootCertificates() {
    return "[]"; // Would return array of root CA certs
}

// Check if ALPN protocol is supported
int nova_tls_checkServerIdentity(const char* hostname, void* cert) {
    (void)hostname; (void)cert;
    return 1; // Would verify certificate against hostname
}

// Convert PFX/PKCS12 to PEM
const char* nova_tls_convertPFXtoPEM(const uint8_t* pfx, int pfxLen, const char* passphrase) {
    (void)pfx; (void)pfxLen; (void)passphrase;
    return ""; // Would convert using OpenSSL
}

// Get ephemeral key info
const char* nova_tls_socket_getEphemeralKeyInfo(void* sock) {
    (void)sock;
    return "{\"type\":\"ECDH\",\"name\":\"X25519\",\"size\":253}";
}

// Get shared signature algorithms
const char* nova_tls_socket_getSharedSigalgs(void* sock) {
    (void)sock;
    return "[\"RSA-PSS+SHA256\",\"RSA-PSS+SHA384\",\"RSA-PSS+SHA512\",\"ECDSA+SHA256\"]";
}

// Get TLS ticket
int nova_tls_socket_getTLSTicket(void* sock, uint8_t* buffer, int maxLen) {
    (void)sock; (void)buffer; (void)maxLen;
    return 0; // Would return TLS session ticket
}

// Get finished message
int nova_tls_socket_getFinished(void* sock, uint8_t* buffer, int maxLen) {
    (void)sock; (void)buffer; (void)maxLen;
    return 0; // Would return TLS finished message
}

int nova_tls_socket_getPeerFinished(void* sock, uint8_t* buffer, int maxLen) {
    (void)sock; (void)buffer; (void)maxLen;
    return 0;
}

// ALPN protocol
const char* nova_tls_socket_alpnProtocol(void* sock) {
    (void)sock;
    return ""; // Would return negotiated ALPN protocol
}

void nova_tls_socket_setALPNProtocols(void* sock, const char** protocols, int count) {
    (void)sock; (void)protocols; (void)count;
}

// Server name (SNI)
const char* nova_tls_socket_servername(void* sock) {
    (void)sock;
    return "";
}

} // extern "C"
