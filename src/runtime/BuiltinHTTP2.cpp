/**
 * nova:http2 - HTTP/2 Module Implementation
 *
 * Provides HTTP/2 server and client for Nova programs.
 * Compatible with Node.js http2 module.
 */

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
namespace http2 {

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
// HTTP/2 Constants
// ============================================================================

// Error codes
static const int NGHTTP2_NO_ERROR = 0x00;
static const int NGHTTP2_PROTOCOL_ERROR = 0x01;
static const int NGHTTP2_INTERNAL_ERROR = 0x02;
static const int NGHTTP2_FLOW_CONTROL_ERROR = 0x03;
static const int NGHTTP2_SETTINGS_TIMEOUT = 0x04;
static const int NGHTTP2_STREAM_CLOSED = 0x05;
static const int NGHTTP2_FRAME_SIZE_ERROR = 0x06;
static const int NGHTTP2_REFUSED_STREAM = 0x07;
static const int NGHTTP2_CANCEL = 0x08;
static const int NGHTTP2_COMPRESSION_ERROR = 0x09;
static const int NGHTTP2_CONNECT_ERROR = 0x0a;
static const int NGHTTP2_ENHANCE_YOUR_CALM = 0x0b;
static const int NGHTTP2_INADEQUATE_SECURITY = 0x0c;
static const int NGHTTP2_HTTP_1_1_REQUIRED = 0x0d;

// Settings
static const int NGHTTP2_SETTINGS_HEADER_TABLE_SIZE = 0x01;
static const int NGHTTP2_SETTINGS_ENABLE_PUSH = 0x02;
static const int NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS = 0x03;
static const int NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE = 0x04;
static const int NGHTTP2_SETTINGS_MAX_FRAME_SIZE = 0x05;
static const int NGHTTP2_SETTINGS_MAX_HEADER_LIST_SIZE = 0x06;
static const int NGHTTP2_SETTINGS_ENABLE_CONNECT_PROTOCOL = 0x08;

// Default settings
static const int DEFAULT_HEADER_TABLE_SIZE = 4096;
static const int DEFAULT_ENABLE_PUSH = 1;
static const int DEFAULT_MAX_CONCURRENT_STREAMS = 100;
static const int DEFAULT_INITIAL_WINDOW_SIZE = 65535;
static const int DEFAULT_MAX_FRAME_SIZE = 16384;
static const int DEFAULT_MAX_HEADER_LIST_SIZE = 65535;

// ============================================================================
// Settings Structure
// ============================================================================

struct Http2Settings {
    int headerTableSize;
    int enablePush;
    int maxConcurrentStreams;
    int initialWindowSize;
    int maxFrameSize;
    int maxHeaderListSize;
    int enableConnectProtocol;
};

static Http2Settings defaultSettings = {
    DEFAULT_HEADER_TABLE_SIZE,
    DEFAULT_ENABLE_PUSH,
    DEFAULT_MAX_CONCURRENT_STREAMS,
    DEFAULT_INITIAL_WINDOW_SIZE,
    DEFAULT_MAX_FRAME_SIZE,
    DEFAULT_MAX_HEADER_LIST_SIZE,
    0
};

// ============================================================================
// Http2Stream Structure
// ============================================================================

struct Http2Stream {
    int id;
    int state;  // 0=idle, 1=open, 2=reserved, 3=half-closed, 4=closed
    int weight;
    int exclusive;
    int sentHeaders;
    int sentTrailers;
    int endAfterHeaders;
    int aborted;
    int closed;
    int destroyed;
    std::map<std::string, std::string> headers;
    void* session;
    void (*onData)(void* stream, const char* data, int len);
    void (*onEnd)(void* stream);
    void (*onError)(void* stream, const char* error);
    void (*onClose)(void* stream, int code);
};

// ============================================================================
// Http2Session Structure
// ============================================================================

struct Http2Session {
    int type;  // 0=server, 1=client
    socket_t socket;
    int destroyed;
    int closed;
    int connecting;
    int localSettings[7];
    int remoteSettings[7];
    std::vector<Http2Stream*> streams;
    int nextStreamId;
    void (*onStream)(void* session, void* stream, void* headers, int flags);
    void (*onError)(void* session, const char* error);
    void (*onClose)(void* session);
    void (*onConnect)(void* session);
    void (*onGoaway)(void* session, int code, int lastStreamId);
    void (*onPing)(void* session);
    void (*onSettings)(void* session);
    void (*onTimeout)(void* session);
};

// ============================================================================
// Http2Server Structure
// ============================================================================

struct Http2Server {
    socket_t socket;
    int listening;
    int port;
    char* hostname;
    int timeout;
    int maxSessionMemory;
    std::vector<Http2Session*> sessions;
    void (*onSession)(void* server, void* session);
    void (*onRequest)(void* server, void* request, void* response);
    void (*onError)(void* server, const char* error);
    void (*onClose)(void* server);
    void (*onListening)(void* server);
    void (*onCheckContinue)(void* server, void* request, void* response);
    void (*onStream)(void* server, void* stream, void* headers, int flags);
};

// ============================================================================
// Http2ServerRequest Structure
// ============================================================================

struct Http2ServerRequest {
    Http2Stream* stream;
    char* method;
    char* authority;
    char* scheme;
    char* path;
    std::map<std::string, std::string> headers;
    int complete;
    int aborted;
    char* httpVersion;
};

// ============================================================================
// Http2ServerResponse Structure
// ============================================================================

struct Http2ServerResponse {
    Http2Stream* stream;
    int statusCode;
    std::map<std::string, std::string> headers;
    int headersSent;
    int finished;
    int closed;
};

extern "C" {

// ============================================================================
// Constants Export
// ============================================================================

int nova_http2_constants_NO_ERROR() { return NGHTTP2_NO_ERROR; }
int nova_http2_constants_PROTOCOL_ERROR() { return NGHTTP2_PROTOCOL_ERROR; }
int nova_http2_constants_INTERNAL_ERROR() { return NGHTTP2_INTERNAL_ERROR; }
int nova_http2_constants_FLOW_CONTROL_ERROR() { return NGHTTP2_FLOW_CONTROL_ERROR; }
int nova_http2_constants_SETTINGS_TIMEOUT() { return NGHTTP2_SETTINGS_TIMEOUT; }
int nova_http2_constants_STREAM_CLOSED() { return NGHTTP2_STREAM_CLOSED; }
int nova_http2_constants_FRAME_SIZE_ERROR() { return NGHTTP2_FRAME_SIZE_ERROR; }
int nova_http2_constants_REFUSED_STREAM() { return NGHTTP2_REFUSED_STREAM; }
int nova_http2_constants_CANCEL() { return NGHTTP2_CANCEL; }
int nova_http2_constants_COMPRESSION_ERROR() { return NGHTTP2_COMPRESSION_ERROR; }
int nova_http2_constants_CONNECT_ERROR() { return NGHTTP2_CONNECT_ERROR; }
int nova_http2_constants_ENHANCE_YOUR_CALM() { return NGHTTP2_ENHANCE_YOUR_CALM; }
int nova_http2_constants_INADEQUATE_SECURITY() { return NGHTTP2_INADEQUATE_SECURITY; }
int nova_http2_constants_HTTP_1_1_REQUIRED() { return NGHTTP2_HTTP_1_1_REQUIRED; }

int nova_http2_constants_HEADER_TABLE_SIZE() { return NGHTTP2_SETTINGS_HEADER_TABLE_SIZE; }
int nova_http2_constants_ENABLE_PUSH() { return NGHTTP2_SETTINGS_ENABLE_PUSH; }
int nova_http2_constants_MAX_CONCURRENT_STREAMS() { return NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS; }
int nova_http2_constants_INITIAL_WINDOW_SIZE() { return NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE; }
int nova_http2_constants_MAX_FRAME_SIZE() { return NGHTTP2_SETTINGS_MAX_FRAME_SIZE; }
int nova_http2_constants_MAX_HEADER_LIST_SIZE() { return NGHTTP2_SETTINGS_MAX_HEADER_LIST_SIZE; }
int nova_http2_constants_ENABLE_CONNECT_PROTOCOL() { return NGHTTP2_SETTINGS_ENABLE_CONNECT_PROTOCOL; }

int nova_http2_constants_DEFAULT_HEADER_TABLE_SIZE() { return DEFAULT_HEADER_TABLE_SIZE; }
int nova_http2_constants_DEFAULT_ENABLE_PUSH() { return DEFAULT_ENABLE_PUSH; }
int nova_http2_constants_DEFAULT_MAX_CONCURRENT_STREAMS() { return DEFAULT_MAX_CONCURRENT_STREAMS; }
int nova_http2_constants_DEFAULT_INITIAL_WINDOW_SIZE() { return DEFAULT_INITIAL_WINDOW_SIZE; }
int nova_http2_constants_DEFAULT_MAX_FRAME_SIZE() { return DEFAULT_MAX_FRAME_SIZE; }
int nova_http2_constants_DEFAULT_MAX_HEADER_LIST_SIZE() { return DEFAULT_MAX_HEADER_LIST_SIZE; }

// ============================================================================
// Settings Functions
// ============================================================================

// Get default settings
void* nova_http2_getDefaultSettings() {
    Http2Settings* settings = new Http2Settings();
    *settings = defaultSettings;
    return settings;
}

// Get packed settings (buffer format)
char* nova_http2_getPackedSettings(void* settingsPtr, int* length) {
    Http2Settings* settings = settingsPtr ? (Http2Settings*)settingsPtr : &defaultSettings;

    // Each setting is 6 bytes: 2 bytes ID + 4 bytes value
    *length = 6 * 6;  // 6 settings
    char* packed = (char*)malloc(*length);

    int offset = 0;
    auto writeSetting = [&](int id, int value) {
        packed[offset++] = (id >> 8) & 0xFF;
        packed[offset++] = id & 0xFF;
        packed[offset++] = (value >> 24) & 0xFF;
        packed[offset++] = (value >> 16) & 0xFF;
        packed[offset++] = (value >> 8) & 0xFF;
        packed[offset++] = value & 0xFF;
    };

    writeSetting(NGHTTP2_SETTINGS_HEADER_TABLE_SIZE, settings->headerTableSize);
    writeSetting(NGHTTP2_SETTINGS_ENABLE_PUSH, settings->enablePush);
    writeSetting(NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, settings->maxConcurrentStreams);
    writeSetting(NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, settings->initialWindowSize);
    writeSetting(NGHTTP2_SETTINGS_MAX_FRAME_SIZE, settings->maxFrameSize);
    writeSetting(NGHTTP2_SETTINGS_MAX_HEADER_LIST_SIZE, settings->maxHeaderListSize);

    return packed;
}

// Get unpacked settings from buffer
void* nova_http2_getUnpackedSettings(const char* buffer, int length) {
    Http2Settings* settings = new Http2Settings();
    *settings = defaultSettings;

    for (int i = 0; i + 5 < length; i += 6) {
        int id = ((unsigned char)buffer[i] << 8) | (unsigned char)buffer[i + 1];
        int value = ((unsigned char)buffer[i + 2] << 24) |
                   ((unsigned char)buffer[i + 3] << 16) |
                   ((unsigned char)buffer[i + 4] << 8) |
                   (unsigned char)buffer[i + 5];

        switch (id) {
            case NGHTTP2_SETTINGS_HEADER_TABLE_SIZE:
                settings->headerTableSize = value;
                break;
            case NGHTTP2_SETTINGS_ENABLE_PUSH:
                settings->enablePush = value;
                break;
            case NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS:
                settings->maxConcurrentStreams = value;
                break;
            case NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE:
                settings->initialWindowSize = value;
                break;
            case NGHTTP2_SETTINGS_MAX_FRAME_SIZE:
                settings->maxFrameSize = value;
                break;
            case NGHTTP2_SETTINGS_MAX_HEADER_LIST_SIZE:
                settings->maxHeaderListSize = value;
                break;
        }
    }

    return settings;
}

void nova_http2_freeSettings(void* settingsPtr) {
    if (settingsPtr) {
        delete (Http2Settings*)settingsPtr;
    }
}

// ============================================================================
// Http2Session Functions
// ============================================================================

void* nova_http2_Session_new(int type) {
    Http2Session* session = new Http2Session();
    session->type = type;
    session->socket = INVALID_SOCK;
    session->destroyed = 0;
    session->closed = 0;
    session->connecting = 0;
    session->nextStreamId = type == 1 ? 1 : 2;  // Client=odd, Server=even

    // Initialize settings
    session->localSettings[0] = DEFAULT_HEADER_TABLE_SIZE;
    session->localSettings[1] = DEFAULT_ENABLE_PUSH;
    session->localSettings[2] = DEFAULT_MAX_CONCURRENT_STREAMS;
    session->localSettings[3] = DEFAULT_INITIAL_WINDOW_SIZE;
    session->localSettings[4] = DEFAULT_MAX_FRAME_SIZE;
    session->localSettings[5] = DEFAULT_MAX_HEADER_LIST_SIZE;
    session->localSettings[6] = 0;

    for (int i = 0; i < 7; i++) {
        session->remoteSettings[i] = session->localSettings[i];
    }

    session->onStream = nullptr;
    session->onError = nullptr;
    session->onClose = nullptr;
    session->onConnect = nullptr;
    session->onGoaway = nullptr;
    session->onPing = nullptr;
    session->onSettings = nullptr;
    session->onTimeout = nullptr;

    return session;
}

int nova_http2_Session_type(void* sessionPtr) {
    if (!sessionPtr) return -1;
    return ((Http2Session*)sessionPtr)->type;
}

int nova_http2_Session_destroyed(void* sessionPtr) {
    if (!sessionPtr) return 1;
    return ((Http2Session*)sessionPtr)->destroyed;
}

int nova_http2_Session_closed(void* sessionPtr) {
    if (!sessionPtr) return 1;
    return ((Http2Session*)sessionPtr)->closed;
}

int nova_http2_Session_connecting(void* sessionPtr) {
    if (!sessionPtr) return 0;
    return ((Http2Session*)sessionPtr)->connecting;
}

void nova_http2_Session_settings(void* sessionPtr, void* settingsPtr) {
    if (!sessionPtr) return;

    Http2Session* session = (Http2Session*)sessionPtr;
    Http2Settings* settings = (Http2Settings*)settingsPtr;

    if (settings) {
        session->localSettings[0] = settings->headerTableSize;
        session->localSettings[1] = settings->enablePush;
        session->localSettings[2] = settings->maxConcurrentStreams;
        session->localSettings[3] = settings->initialWindowSize;
        session->localSettings[4] = settings->maxFrameSize;
        session->localSettings[5] = settings->maxHeaderListSize;
        session->localSettings[6] = settings->enableConnectProtocol;
    }
}

int nova_http2_Session_localSettings(void* sessionPtr, int index) {
    if (!sessionPtr || index < 0 || index > 6) return 0;
    return ((Http2Session*)sessionPtr)->localSettings[index];
}

int nova_http2_Session_remoteSettings(void* sessionPtr, int index) {
    if (!sessionPtr || index < 0 || index > 6) return 0;
    return ((Http2Session*)sessionPtr)->remoteSettings[index];
}

void nova_http2_Session_ping(void* sessionPtr, void* callback) {
    if (!sessionPtr) return;
    Http2Session* session = (Http2Session*)sessionPtr;
    // Send PING frame (simplified)
    if (session->onPing) {
        session->onPing(sessionPtr);
    }
    (void)callback;
}

void nova_http2_Session_goaway(void* sessionPtr, int code, int lastStreamId) {
    if (!sessionPtr) return;
    Http2Session* session = (Http2Session*)sessionPtr;
    session->closed = 1;
    if (session->onGoaway) {
        session->onGoaway(sessionPtr, code, lastStreamId);
    }
}

void nova_http2_Session_close(void* sessionPtr, void* callback) {
    if (!sessionPtr) return;
    Http2Session* session = (Http2Session*)sessionPtr;

    session->closed = 1;

    // Close all streams
    for (auto& stream : session->streams) {
        stream->closed = 1;
    }

    if (session->socket != INVALID_SOCK) {
        CLOSE_SOCKET(session->socket);
        session->socket = INVALID_SOCK;
    }

    if (session->onClose) {
        session->onClose(sessionPtr);
    }

    if (callback) {
        void (*cb)(void*) = (void (*)(void*))callback;
        cb(sessionPtr);
    }
}

void nova_http2_Session_destroy(void* sessionPtr, int code) {
    if (!sessionPtr) return;
    Http2Session* session = (Http2Session*)sessionPtr;

    session->destroyed = 1;
    nova_http2_Session_close(sessionPtr, nullptr);
    (void)code;
}

void nova_http2_Session_on(void* sessionPtr, const char* event, void* handler) {
    if (!sessionPtr || !event) return;
    Http2Session* session = (Http2Session*)sessionPtr;

    if (strcmp(event, "stream") == 0) {
        session->onStream = (void (*)(void*, void*, void*, int))handler;
    } else if (strcmp(event, "error") == 0) {
        session->onError = (void (*)(void*, const char*))handler;
    } else if (strcmp(event, "close") == 0) {
        session->onClose = (void (*)(void*))handler;
    } else if (strcmp(event, "connect") == 0) {
        session->onConnect = (void (*)(void*))handler;
    } else if (strcmp(event, "goaway") == 0) {
        session->onGoaway = (void (*)(void*, int, int))handler;
    } else if (strcmp(event, "ping") == 0) {
        session->onPing = (void (*)(void*))handler;
    } else if (strcmp(event, "localSettings") == 0 || strcmp(event, "remoteSettings") == 0) {
        session->onSettings = (void (*)(void*))handler;
    } else if (strcmp(event, "timeout") == 0) {
        session->onTimeout = (void (*)(void*))handler;
    }
}

void nova_http2_Session_free(void* sessionPtr) {
    if (!sessionPtr) return;
    Http2Session* session = (Http2Session*)sessionPtr;

    for (auto& stream : session->streams) {
        delete stream;
    }
    session->streams.clear();

    delete session;
}

// ============================================================================
// Http2Stream Functions
// ============================================================================

void* nova_http2_Stream_new(void* sessionPtr) {
    if (!sessionPtr) return nullptr;

    Http2Session* session = (Http2Session*)sessionPtr;
    Http2Stream* stream = new Http2Stream();

    stream->id = session->nextStreamId;
    session->nextStreamId += 2;

    stream->state = 0;
    stream->weight = 16;
    stream->exclusive = 0;
    stream->sentHeaders = 0;
    stream->sentTrailers = 0;
    stream->endAfterHeaders = 0;
    stream->aborted = 0;
    stream->closed = 0;
    stream->destroyed = 0;
    stream->session = sessionPtr;
    stream->onData = nullptr;
    stream->onEnd = nullptr;
    stream->onError = nullptr;
    stream->onClose = nullptr;

    session->streams.push_back(stream);
    return stream;
}

int nova_http2_Stream_id(void* streamPtr) {
    if (!streamPtr) return 0;
    return ((Http2Stream*)streamPtr)->id;
}

int nova_http2_Stream_state(void* streamPtr) {
    if (!streamPtr) return 4;  // closed
    return ((Http2Stream*)streamPtr)->state;
}

int nova_http2_Stream_closed(void* streamPtr) {
    if (!streamPtr) return 1;
    return ((Http2Stream*)streamPtr)->closed;
}

int nova_http2_Stream_destroyed(void* streamPtr) {
    if (!streamPtr) return 1;
    return ((Http2Stream*)streamPtr)->destroyed;
}

int nova_http2_Stream_sentHeaders(void* streamPtr) {
    if (!streamPtr) return 0;
    return ((Http2Stream*)streamPtr)->sentHeaders;
}

int nova_http2_Stream_sentTrailers(void* streamPtr) {
    if (!streamPtr) return 0;
    return ((Http2Stream*)streamPtr)->sentTrailers;
}

void* nova_http2_Stream_session(void* streamPtr) {
    if (!streamPtr) return nullptr;
    return ((Http2Stream*)streamPtr)->session;
}

void nova_http2_Stream_priority(void* streamPtr, int weight, int exclusive) {
    if (!streamPtr) return;
    Http2Stream* stream = (Http2Stream*)streamPtr;
    stream->weight = weight > 0 && weight <= 256 ? weight : 16;
    stream->exclusive = exclusive ? 1 : 0;
}

void nova_http2_Stream_respond(void* streamPtr, int statusCode, const char** headers, int headerCount) {
    if (!streamPtr) return;
    Http2Stream* stream = (Http2Stream*)streamPtr;

    stream->headers[":status"] = std::to_string(statusCode);

    for (int i = 0; i + 1 < headerCount; i += 2) {
        if (headers[i] && headers[i + 1]) {
            stream->headers[headers[i]] = headers[i + 1];
        }
    }

    stream->sentHeaders = 1;
    stream->state = 1;  // open
}

int nova_http2_Stream_write(void* streamPtr, const char* data, int length) {
    if (!streamPtr || !data) return 0;
    Http2Stream* stream = (Http2Stream*)streamPtr;

    if (stream->closed || stream->destroyed) return 0;

    // In a real implementation, this would send DATA frames
    (void)length;
    return 1;
}

void nova_http2_Stream_end(void* streamPtr, const char* data, int length) {
    if (!streamPtr) return;
    Http2Stream* stream = (Http2Stream*)streamPtr;

    if (data) {
        nova_http2_Stream_write(streamPtr, data, length);
    }

    stream->state = 3;  // half-closed (local)

    if (stream->onEnd) {
        stream->onEnd(streamPtr);
    }
}

void nova_http2_Stream_close(void* streamPtr, int code) {
    if (!streamPtr) return;
    Http2Stream* stream = (Http2Stream*)streamPtr;

    stream->closed = 1;
    stream->state = 4;  // closed

    if (stream->onClose) {
        stream->onClose(streamPtr, code);
    }
}

void nova_http2_Stream_rstStream(void* streamPtr, int code) {
    nova_http2_Stream_close(streamPtr, code);
}

void nova_http2_Stream_on(void* streamPtr, const char* event, void* handler) {
    if (!streamPtr || !event) return;
    Http2Stream* stream = (Http2Stream*)streamPtr;

    if (strcmp(event, "data") == 0) {
        stream->onData = (void (*)(void*, const char*, int))handler;
    } else if (strcmp(event, "end") == 0) {
        stream->onEnd = (void (*)(void*))handler;
    } else if (strcmp(event, "error") == 0) {
        stream->onError = (void (*)(void*, const char*))handler;
    } else if (strcmp(event, "close") == 0) {
        stream->onClose = (void (*)(void*, int))handler;
    }
}

void nova_http2_Stream_free(void* streamPtr) {
    if (!streamPtr) return;
    delete (Http2Stream*)streamPtr;
}

// ============================================================================
// Http2Server Functions
// ============================================================================

void* nova_http2_createServer(void* requestHandler) {
#ifdef _WIN32
    initWinsock();
#endif

    Http2Server* server = new Http2Server();
    server->socket = INVALID_SOCK;
    server->listening = 0;
    server->port = 0;
    server->hostname = nullptr;
    server->timeout = 0;
    server->maxSessionMemory = 10 * 1024 * 1024;  // 10MB
    server->onSession = nullptr;
    server->onRequest = (void (*)(void*, void*, void*))requestHandler;
    server->onError = nullptr;
    server->onClose = nullptr;
    server->onListening = nullptr;
    server->onCheckContinue = nullptr;
    server->onStream = nullptr;

    return server;
}

void* nova_http2_createSecureServer(void* options, void* requestHandler) {
    // For now, same as createServer (TLS would require additional setup)
    (void)options;
    return nova_http2_createServer(requestHandler);
}

int nova_http2_Server_listen(void* serverPtr, int port, const char* hostname, void* callback) {
    if (!serverPtr) return 0;

    Http2Server* server = (Http2Server*)serverPtr;

#ifdef _WIN32
    initWinsock();
#endif

    server->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server->socket == INVALID_SOCK) {
        if (server->onError) {
            server->onError(serverPtr, "Failed to create socket");
        }
        return 0;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(server->socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
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

    if (bind(server->socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        if (server->onError) {
            server->onError(serverPtr, "Failed to bind");
        }
        CLOSE_SOCKET(server->socket);
        return 0;
    }

    if (listen(server->socket, SOMAXCONN) < 0) {
        if (server->onError) {
            server->onError(serverPtr, "Failed to listen");
        }
        CLOSE_SOCKET(server->socket);
        return 0;
    }

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

void nova_http2_Server_close(void* serverPtr, void* callback) {
    if (!serverPtr) return;

    Http2Server* server = (Http2Server*)serverPtr;

    // Close all sessions
    for (auto& session : server->sessions) {
        nova_http2_Session_close(session, nullptr);
    }

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

int nova_http2_Server_listening(void* serverPtr) {
    if (!serverPtr) return 0;
    return ((Http2Server*)serverPtr)->listening;
}

void nova_http2_Server_setTimeout(void* serverPtr, int ms, void* callback) {
    if (!serverPtr) return;
    ((Http2Server*)serverPtr)->timeout = ms;
    (void)callback;
}

void nova_http2_Server_on(void* serverPtr, const char* event, void* handler) {
    if (!serverPtr || !event) return;
    Http2Server* server = (Http2Server*)serverPtr;

    if (strcmp(event, "session") == 0) {
        server->onSession = (void (*)(void*, void*))handler;
    } else if (strcmp(event, "request") == 0) {
        server->onRequest = (void (*)(void*, void*, void*))handler;
    } else if (strcmp(event, "error") == 0) {
        server->onError = (void (*)(void*, const char*))handler;
    } else if (strcmp(event, "close") == 0) {
        server->onClose = (void (*)(void*))handler;
    } else if (strcmp(event, "listening") == 0) {
        server->onListening = (void (*)(void*))handler;
    } else if (strcmp(event, "checkContinue") == 0) {
        server->onCheckContinue = (void (*)(void*, void*, void*))handler;
    } else if (strcmp(event, "stream") == 0) {
        server->onStream = (void (*)(void*, void*, void*, int))handler;
    }
}

void nova_http2_Server_free(void* serverPtr) {
    if (!serverPtr) return;
    Http2Server* server = (Http2Server*)serverPtr;

    nova_http2_Server_close(serverPtr, nullptr);

    for (auto& session : server->sessions) {
        nova_http2_Session_free(session);
    }

    if (server->hostname) free(server->hostname);
    delete server;
}

// ============================================================================
// Client Connect
// ============================================================================

void* nova_http2_connect(const char* authority, void* options, void* listener) {
#ifdef _WIN32
    initWinsock();
#endif

    Http2Session* session = (Http2Session*)nova_http2_Session_new(1);
    session->connecting = 1;

    // Parse authority (host:port)
    std::string auth = authority ? authority : "localhost:80";
    std::string host = auth;
    int port = 80;

    size_t colonPos = auth.find(':');
    if (colonPos != std::string::npos) {
        host = auth.substr(0, colonPos);
        port = atoi(auth.substr(colonPos + 1).c_str());
    }

    // Connect
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char portStr[16];
    snprintf(portStr, sizeof(portStr), "%d", port);

    if (getaddrinfo(host.c_str(), portStr, &hints, &res) != 0) {
        session->connecting = 0;
        if (session->onError) {
            session->onError(session, "Failed to resolve host");
        }
        return session;
    }

    session->socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (session->socket == INVALID_SOCK) {
        freeaddrinfo(res);
        session->connecting = 0;
        if (session->onError) {
            session->onError(session, "Failed to create socket");
        }
        return session;
    }

    if (connect(session->socket, res->ai_addr, (int)res->ai_addrlen) < 0) {
        CLOSE_SOCKET(session->socket);
        session->socket = INVALID_SOCK;
        freeaddrinfo(res);
        session->connecting = 0;
        if (session->onError) {
            session->onError(session, "Failed to connect");
        }
        return session;
    }

    freeaddrinfo(res);
    session->connecting = 0;

    if (listener) {
        void (*cb)(void*) = (void (*)(void*))listener;
        cb(session);
    }

    if (session->onConnect) {
        session->onConnect(session);
    }

    (void)options;
    return session;
}

// Client request
void* nova_http2_ClientSession_request(void* sessionPtr, const char** headers, int headerCount) {
    if (!sessionPtr) return nullptr;

    Http2Stream* stream = (Http2Stream*)nova_http2_Stream_new(sessionPtr);

    for (int i = 0; i + 1 < headerCount; i += 2) {
        if (headers[i] && headers[i + 1]) {
            stream->headers[headers[i]] = headers[i + 1];
        }
    }

    stream->state = 1;  // open
    return stream;
}

// ============================================================================
// Http2ServerRequest Functions
// ============================================================================

void* nova_http2_ServerRequest_new(void* streamPtr) {
    Http2ServerRequest* req = new Http2ServerRequest();
    req->stream = (Http2Stream*)streamPtr;
    req->method = allocString("GET");
    req->authority = nullptr;
    req->scheme = allocString("http");
    req->path = allocString("/");
    req->complete = 0;
    req->aborted = 0;
    req->httpVersion = allocString("2.0");
    return req;
}

char* nova_http2_ServerRequest_method(void* reqPtr) {
    if (!reqPtr) return nullptr;
    Http2ServerRequest* req = (Http2ServerRequest*)reqPtr;
    return req->method ? allocString(req->method) : nullptr;
}

char* nova_http2_ServerRequest_authority(void* reqPtr) {
    if (!reqPtr) return nullptr;
    Http2ServerRequest* req = (Http2ServerRequest*)reqPtr;
    return req->authority ? allocString(req->authority) : nullptr;
}

char* nova_http2_ServerRequest_scheme(void* reqPtr) {
    if (!reqPtr) return nullptr;
    Http2ServerRequest* req = (Http2ServerRequest*)reqPtr;
    return req->scheme ? allocString(req->scheme) : nullptr;
}

char* nova_http2_ServerRequest_path(void* reqPtr) {
    if (!reqPtr) return nullptr;
    Http2ServerRequest* req = (Http2ServerRequest*)reqPtr;
    return req->path ? allocString(req->path) : nullptr;
}

char* nova_http2_ServerRequest_httpVersion(void* reqPtr) {
    if (!reqPtr) return allocString("2.0");
    Http2ServerRequest* req = (Http2ServerRequest*)reqPtr;
    return req->httpVersion ? allocString(req->httpVersion) : allocString("2.0");
}

void* nova_http2_ServerRequest_stream(void* reqPtr) {
    if (!reqPtr) return nullptr;
    return ((Http2ServerRequest*)reqPtr)->stream;
}

void nova_http2_ServerRequest_free(void* reqPtr) {
    if (!reqPtr) return;
    Http2ServerRequest* req = (Http2ServerRequest*)reqPtr;
    if (req->method) free(req->method);
    if (req->authority) free(req->authority);
    if (req->scheme) free(req->scheme);
    if (req->path) free(req->path);
    if (req->httpVersion) free(req->httpVersion);
    delete req;
}

// ============================================================================
// Http2ServerResponse Functions
// ============================================================================

void* nova_http2_ServerResponse_new(void* streamPtr) {
    Http2ServerResponse* res = new Http2ServerResponse();
    res->stream = (Http2Stream*)streamPtr;
    res->statusCode = 200;
    res->headersSent = 0;
    res->finished = 0;
    res->closed = 0;
    return res;
}

void nova_http2_ServerResponse_setStatusCode(void* resPtr, int code) {
    if (!resPtr) return;
    ((Http2ServerResponse*)resPtr)->statusCode = code;
}

int nova_http2_ServerResponse_statusCode(void* resPtr) {
    if (!resPtr) return 200;
    return ((Http2ServerResponse*)resPtr)->statusCode;
}

void nova_http2_ServerResponse_setHeader(void* resPtr, const char* name, const char* value) {
    if (!resPtr || !name || !value) return;
    Http2ServerResponse* res = (Http2ServerResponse*)resPtr;
    if (res->headersSent) return;
    res->headers[name] = value;
}

int nova_http2_ServerResponse_write(void* resPtr, const char* data, int length) {
    if (!resPtr) return 0;
    Http2ServerResponse* res = (Http2ServerResponse*)resPtr;
    if (res->finished) return 0;

    if (!res->headersSent && res->stream) {
        // Build headers array
        std::vector<const char*> headerPtrs;
        headerPtrs.push_back(":status");
        std::string statusStr = std::to_string(res->statusCode);
        headerPtrs.push_back(statusStr.c_str());

        for (auto& pair : res->headers) {
            headerPtrs.push_back(pair.first.c_str());
            headerPtrs.push_back(pair.second.c_str());
        }

        nova_http2_Stream_respond(res->stream, res->statusCode,
                                  headerPtrs.data(), (int)headerPtrs.size());
        res->headersSent = 1;
    }

    if (data && res->stream) {
        return nova_http2_Stream_write(res->stream, data, length);
    }
    return 1;
}

void nova_http2_ServerResponse_end(void* resPtr, const char* data, int length) {
    if (!resPtr) return;
    Http2ServerResponse* res = (Http2ServerResponse*)resPtr;
    if (res->finished) return;

    if (data) {
        nova_http2_ServerResponse_write(resPtr, data, length);
    } else if (!res->headersSent) {
        nova_http2_ServerResponse_write(resPtr, "", 0);
    }

    if (res->stream) {
        nova_http2_Stream_end(res->stream, nullptr, 0);
    }

    res->finished = 1;
}

int nova_http2_ServerResponse_finished(void* resPtr) {
    if (!resPtr) return 1;
    return ((Http2ServerResponse*)resPtr)->finished;
}

void* nova_http2_ServerResponse_stream(void* resPtr) {
    if (!resPtr) return nullptr;
    return ((Http2ServerResponse*)resPtr)->stream;
}

void nova_http2_ServerResponse_free(void* resPtr) {
    if (!resPtr) return;
    delete (Http2ServerResponse*)resPtr;
}

// ============================================================================
// Sensitive Headers Symbol
// ============================================================================

void* nova_http2_sensitiveHeaders() {
    static int sensitiveHeadersSymbol = 0x5E45;
    return &sensitiveHeadersSymbol;
}

// ============================================================================
// Utility Functions
// ============================================================================

void nova_http2_cleanup() {
#ifdef _WIN32
    if (wsaInitialized) {
        WSACleanup();
        wsaInitialized = false;
    }
#endif
}

} // extern "C"

} // namespace http2
} // namespace runtime
} // namespace nova
