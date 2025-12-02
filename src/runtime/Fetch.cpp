// Fetch Runtime Implementation for Nova Compiler
// Web APIs: fetch, Request, Response, Headers

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
// OpenSSL support for HTTPS on POSIX
#ifdef NOVA_HAS_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif
#endif

// ============================================================================
// Headers Structure (outside extern "C" due to C++ members)
// ============================================================================

struct NovaHeaders {
    std::map<std::string, std::string>* headers;
};

// ============================================================================
// Request Structure
// ============================================================================

struct NovaRequest {
    char* url;
    char* method;
    NovaHeaders* headers;
    char* body;
    int64_t bodyLength;
    char* mode;
    char* credentials;
    char* cache;
    char* redirect;
    char* referrer;
    char* integrity;
};

// ============================================================================
// Response Structure
// ============================================================================

struct NovaResponse {
    int64_t status;
    char* statusText;
    bool ok;
    NovaHeaders* headers;
    char* body;
    int64_t bodyLength;
    char* url;
    char* type;
    bool redirected;
    bool bodyUsed;
};

// ============================================================================
// Helper: Lowercase string
// ============================================================================

static std::string toLower(const std::string& s) {
    std::string result = s;
    for (char& c : result) {
        c = tolower(c);
    }
    return result;
}

extern "C" {

// ============================================================================
// Headers Constructor
// ============================================================================

void* nova_headers_create() {
    NovaHeaders* h = new NovaHeaders();
    h->headers = new std::map<std::string, std::string>();
    return h;
}

// ============================================================================
// Headers.prototype.append(name, value)
// ============================================================================

void nova_headers_append(void* headersPtr, const char* name, const char* value) {
    if (!headersPtr || !name || !value) return;

    NovaHeaders* h = static_cast<NovaHeaders*>(headersPtr);
    std::string key = toLower(name);

    auto it = h->headers->find(key);
    if (it != h->headers->end()) {
        it->second += ", " + std::string(value);
    } else {
        (*h->headers)[key] = value;
    }
}

// ============================================================================
// Headers.prototype.delete(name)
// ============================================================================

void nova_headers_delete(void* headersPtr, const char* name) {
    if (!headersPtr || !name) return;

    NovaHeaders* h = static_cast<NovaHeaders*>(headersPtr);
    h->headers->erase(toLower(name));
}

// ============================================================================
// Headers.prototype.get(name)
// ============================================================================

const char* nova_headers_get(void* headersPtr, const char* name) {
    if (!headersPtr || !name) return nullptr;

    NovaHeaders* h = static_cast<NovaHeaders*>(headersPtr);
    std::string key = toLower(name);

    auto it = h->headers->find(key);
    if (it != h->headers->end()) {
        static thread_local std::string result;
        result = it->second;
        return result.c_str();
    }
    return nullptr;
}

// ============================================================================
// Headers.prototype.has(name)
// ============================================================================

int64_t nova_headers_has(void* headersPtr, const char* name) {
    if (!headersPtr || !name) return 0;

    NovaHeaders* h = static_cast<NovaHeaders*>(headersPtr);
    return h->headers->count(toLower(name)) > 0 ? 1 : 0;
}

// ============================================================================
// Headers.prototype.set(name, value)
// ============================================================================

void nova_headers_set(void* headersPtr, const char* name, const char* value) {
    if (!headersPtr || !name) return;

    NovaHeaders* h = static_cast<NovaHeaders*>(headersPtr);
    (*h->headers)[toLower(name)] = value ? value : "";
}

// ============================================================================
// Headers.prototype.keys(), values(), entries()
// ============================================================================

const char* nova_headers_keys(void* headersPtr) {
    if (!headersPtr) return "";

    NovaHeaders* h = static_cast<NovaHeaders*>(headersPtr);

    static thread_local std::string result;
    result.clear();

    bool first = true;
    for (const auto& pair : *h->headers) {
        if (!first) result += ",";
        result += pair.first;
        first = false;
    }

    return result.c_str();
}

const char* nova_headers_values(void* headersPtr) {
    if (!headersPtr) return "";

    NovaHeaders* h = static_cast<NovaHeaders*>(headersPtr);

    static thread_local std::string result;
    result.clear();

    bool first = true;
    for (const auto& pair : *h->headers) {
        if (!first) result += ",";
        result += pair.second;
        first = false;
    }

    return result.c_str();
}

void nova_headers_destroy(void* headersPtr) {
    if (!headersPtr) return;

    NovaHeaders* h = static_cast<NovaHeaders*>(headersPtr);
    delete h->headers;
    delete h;
}

// ============================================================================
// Request Constructor
// ============================================================================

void* nova_request_create(const char* url) {
    NovaRequest* req = new NovaRequest();
    memset(req, 0, sizeof(NovaRequest));

    req->url = url ? strdup(url) : strdup("");
    req->method = strdup("GET");
    req->headers = static_cast<NovaHeaders*>(nova_headers_create());
    req->mode = strdup("cors");
    req->credentials = strdup("same-origin");
    req->cache = strdup("default");
    req->redirect = strdup("follow");
    req->referrer = strdup("about:client");
    req->integrity = strdup("");

    return req;
}

void* nova_request_create_with_init(const char* url, const char* method, void* headersPtr, const char* body) {
    NovaRequest* req = static_cast<NovaRequest*>(nova_request_create(url));

    if (method) {
        free(req->method);
        req->method = strdup(method);
    }

    if (headersPtr) {
        nova_headers_destroy(req->headers);
        // Clone headers
        NovaHeaders* srcH = static_cast<NovaHeaders*>(headersPtr);
        req->headers = static_cast<NovaHeaders*>(nova_headers_create());
        for (const auto& pair : *srcH->headers) {
            nova_headers_set(req->headers, pair.first.c_str(), pair.second.c_str());
        }
    }

    if (body) {
        req->body = strdup(body);
        req->bodyLength = strlen(body);
    }

    return req;
}

// Request property getters
const char* nova_request_get_url(void* reqPtr) {
    if (!reqPtr) return "";
    return static_cast<NovaRequest*>(reqPtr)->url;
}

const char* nova_request_get_method(void* reqPtr) {
    if (!reqPtr) return "GET";
    return static_cast<NovaRequest*>(reqPtr)->method;
}

void* nova_request_get_headers(void* reqPtr) {
    if (!reqPtr) return nullptr;
    return static_cast<NovaRequest*>(reqPtr)->headers;
}

const char* nova_request_get_body(void* reqPtr) {
    if (!reqPtr) return nullptr;
    return static_cast<NovaRequest*>(reqPtr)->body;
}

void nova_request_destroy(void* reqPtr) {
    if (!reqPtr) return;

    NovaRequest* req = static_cast<NovaRequest*>(reqPtr);
    free(req->url);
    free(req->method);
    nova_headers_destroy(req->headers);
    free(req->body);
    free(req->mode);
    free(req->credentials);
    free(req->cache);
    free(req->redirect);
    free(req->referrer);
    free(req->integrity);
    delete req;
}

// ============================================================================
// Response Constructor
// ============================================================================

void* nova_response_create(const char* body, int64_t status, const char* statusText) {
    NovaResponse* res = new NovaResponse();
    memset(res, 0, sizeof(NovaResponse));

    res->status = status;
    res->statusText = statusText ? strdup(statusText) : strdup("OK");
    res->ok = (status >= 200 && status < 300);
    res->headers = static_cast<NovaHeaders*>(nova_headers_create());
    res->body = body ? strdup(body) : nullptr;
    res->bodyLength = body ? strlen(body) : 0;
    res->url = strdup("");
    res->type = strdup("basic");
    res->redirected = false;
    res->bodyUsed = false;

    return res;
}

// Response property getters
int64_t nova_response_get_status(void* resPtr) {
    if (!resPtr) return 0;
    return static_cast<NovaResponse*>(resPtr)->status;
}

const char* nova_response_get_statusText(void* resPtr) {
    if (!resPtr) return "";
    return static_cast<NovaResponse*>(resPtr)->statusText;
}

int64_t nova_response_get_ok(void* resPtr) {
    if (!resPtr) return 0;
    return static_cast<NovaResponse*>(resPtr)->ok ? 1 : 0;
}

void* nova_response_get_headers(void* resPtr) {
    if (!resPtr) return nullptr;
    return static_cast<NovaResponse*>(resPtr)->headers;
}

const char* nova_response_get_url(void* resPtr) {
    if (!resPtr) return "";
    return static_cast<NovaResponse*>(resPtr)->url;
}

const char* nova_response_get_type(void* resPtr) {
    if (!resPtr) return "basic";
    return static_cast<NovaResponse*>(resPtr)->type;
}

int64_t nova_response_get_redirected(void* resPtr) {
    if (!resPtr) return 0;
    return static_cast<NovaResponse*>(resPtr)->redirected ? 1 : 0;
}

int64_t nova_response_get_bodyUsed(void* resPtr) {
    if (!resPtr) return 0;
    return static_cast<NovaResponse*>(resPtr)->bodyUsed ? 1 : 0;
}

// ============================================================================
// Response.prototype.text()
// ============================================================================

const char* nova_response_text(void* resPtr) {
    if (!resPtr) return "";

    NovaResponse* res = static_cast<NovaResponse*>(resPtr);
    res->bodyUsed = true;

    return res->body ? res->body : "";
}

// ============================================================================
// Response.prototype.json()
// Returns raw JSON string (parsing done in HIR)
// ============================================================================

const char* nova_response_json(void* resPtr) {
    return nova_response_text(resPtr);
}

// ============================================================================
// Response.prototype.blob() - returns as text for now
// ============================================================================

const char* nova_response_blob(void* resPtr) {
    return nova_response_text(resPtr);
}

// ============================================================================
// Response.prototype.arrayBuffer() - returns body data
// ============================================================================

void* nova_response_arrayBuffer(void* resPtr) {
    if (!resPtr) return nullptr;

    NovaResponse* res = static_cast<NovaResponse*>(resPtr);
    res->bodyUsed = true;

    return res->body;
}

int64_t nova_response_arrayBuffer_length(void* resPtr) {
    if (!resPtr) return 0;
    return static_cast<NovaResponse*>(resPtr)->bodyLength;
}

// ============================================================================
// Response.prototype.clone()
// ============================================================================

void* nova_response_clone(void* resPtr) {
    if (!resPtr) return nullptr;

    NovaResponse* src = static_cast<NovaResponse*>(resPtr);

    NovaResponse* clone = new NovaResponse();
    clone->status = src->status;
    clone->statusText = strdup(src->statusText);
    clone->ok = src->ok;
    clone->headers = static_cast<NovaHeaders*>(nova_headers_create());

    // Clone headers
    for (const auto& pair : *src->headers->headers) {
        nova_headers_set(clone->headers, pair.first.c_str(), pair.second.c_str());
    }

    clone->body = src->body ? strdup(src->body) : nullptr;
    clone->bodyLength = src->bodyLength;
    clone->url = strdup(src->url);
    clone->type = strdup(src->type);
    clone->redirected = src->redirected;
    clone->bodyUsed = false;

    return clone;
}

// ============================================================================
// Response Static Methods
// ============================================================================

void* nova_response_error() {
    NovaResponse* res = static_cast<NovaResponse*>(nova_response_create(nullptr, 0, ""));
    free(res->type);
    res->type = strdup("error");
    return res;
}

void* nova_response_redirect(const char* url, int64_t status) {
    if (status == 0) status = 302;

    NovaResponse* res = static_cast<NovaResponse*>(nova_response_create(nullptr, status, ""));
    nova_headers_set(res->headers, "Location", url);
    return res;
}

void* nova_response_json_static(const char* data, int64_t status) {
    if (status == 0) status = 200;

    NovaResponse* res = static_cast<NovaResponse*>(nova_response_create(data, status, "OK"));
    nova_headers_set(res->headers, "Content-Type", "application/json");
    return res;
}

void nova_response_destroy(void* resPtr) {
    if (!resPtr) return;

    NovaResponse* res = static_cast<NovaResponse*>(resPtr);
    free(res->statusText);
    nova_headers_destroy(res->headers);
    free(res->body);
    free(res->url);
    free(res->type);
    delete res;
}

// ============================================================================
// fetch() - Main HTTP request function
// ============================================================================

#ifdef _WIN32

static void* doFetch(const char* url, const char* method, NovaHeaders* headers, const char* body) {
    if (!url) return nova_response_error();

    // Parse URL
    std::string urlStr = url;
    bool isHttps = urlStr.find("https://") == 0;
    std::string hostAndPath = urlStr.substr(isHttps ? 8 : 7);

    size_t pathStart = hostAndPath.find('/');
    std::string host = pathStart != std::string::npos ?
        hostAndPath.substr(0, pathStart) : hostAndPath;
    std::string path = pathStart != std::string::npos ?
        hostAndPath.substr(pathStart) : "/";

    // Extract port if present
    int port = isHttps ? 443 : 80;
    size_t colonPos = host.find(':');
    if (colonPos != std::string::npos) {
        port = std::stoi(host.substr(colonPos + 1));
        host = host.substr(0, colonPos);
    }

    // Convert to wide strings
    std::wstring wHost(host.begin(), host.end());
    std::wstring wPath(path.begin(), path.end());
    std::wstring wMethod = method ?
        std::wstring(method, method + strlen(method)) : L"GET";

    // Open session
    HINTERNET hSession = WinHttpOpen(L"Nova/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    if (!hSession) {
        return nova_response_error();
    }

    // Connect
    HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(),
        static_cast<INTERNET_PORT>(port), 0);

    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return nova_response_error();
    }

    // Open request
    DWORD flags = isHttps ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, wMethod.c_str(),
        wPath.c_str(), nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, flags);

    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return nova_response_error();
    }

    // Add headers
    if (headers) {
        for (const auto& pair : *headers->headers) {
            std::string header = pair.first + ": " + pair.second;
            std::wstring wHeader(header.begin(), header.end());
            WinHttpAddRequestHeaders(hRequest, wHeader.c_str(), (DWORD)-1,
                WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
        }
    }

    // Send request
    DWORD bodyLen = body ? (DWORD)strlen(body) : 0;
    BOOL result = WinHttpSendRequest(hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        body ? (LPVOID)body : WINHTTP_NO_REQUEST_DATA,
        bodyLen, bodyLen, 0);

    if (!result) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return nova_response_error();
    }

    // Receive response
    result = WinHttpReceiveResponse(hRequest, nullptr);

    if (!result) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return nova_response_error();
    }

    // Get status code
    DWORD statusCode = 0;
    DWORD size = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &size,
        WINHTTP_NO_HEADER_INDEX);

    // Read response body
    std::string responseBody;
    DWORD bytesAvailable = 0;

    do {
        bytesAvailable = 0;
        WinHttpQueryDataAvailable(hRequest, &bytesAvailable);

        if (bytesAvailable > 0) {
            char* buffer = new char[bytesAvailable + 1];
            DWORD bytesRead = 0;

            WinHttpReadData(hRequest, buffer, bytesAvailable, &bytesRead);
            buffer[bytesRead] = '\0';
            responseBody += buffer;

            delete[] buffer;
        }
    } while (bytesAvailable > 0);

    // Create response
    NovaResponse* res = static_cast<NovaResponse*>(
        nova_response_create(responseBody.c_str(), statusCode, ""));

    free(res->url);
    res->url = strdup(url);

    // Clean up
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return res;
}

#else

// POSIX implementation with optional OpenSSL support
static void* doFetch(const char* url, const char* method, NovaHeaders* headers, const char* body) {
    if (!url) return nova_response_error();

    std::string urlStr = url;
    bool isHttps = urlStr.find("https://") == 0;
    int defaultPort = isHttps ? 443 : 80;

    // Parse URL
    std::string hostAndPath;
    if (isHttps) {
        hostAndPath = urlStr.substr(8);  // Skip "https://"
    } else {
        hostAndPath = urlStr.substr(7);  // Skip "http://"
    }

    size_t pathStart = hostAndPath.find('/');
    std::string host = pathStart != std::string::npos ?
        hostAndPath.substr(0, pathStart) : hostAndPath;
    std::string path = pathStart != std::string::npos ?
        hostAndPath.substr(pathStart) : "/";

    int port = defaultPort;
    size_t colonPos = host.find(':');
    if (colonPos != std::string::npos) {
        port = std::stoi(host.substr(colonPos + 1));
        host = host.substr(0, colonPos);
    }

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return nova_response_error();
    }

    // Resolve hostname
    struct hostent* server = gethostbyname(host.c_str());
    if (!server) {
        close(sock);
        return nova_response_error();
    }

    // Connect
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    memcpy(&serverAddr.sin_addr.s_addr, server->h_addr, server->h_length);
    serverAddr.sin_port = htons(port);

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(sock);
        return nova_response_error();
    }

#ifdef NOVA_HAS_OPENSSL
    SSL_CTX* sslCtx = nullptr;
    SSL* ssl = nullptr;

    if (isHttps) {
        // Initialize OpenSSL
        static bool sslInitialized = false;
        if (!sslInitialized) {
            SSL_library_init();
            SSL_load_error_strings();
            OpenSSL_add_all_algorithms();
            sslInitialized = true;
        }

        // Create SSL context
        sslCtx = SSL_CTX_new(TLS_client_method());
        if (!sslCtx) {
            close(sock);
            return nova_response_error();
        }

        // Set verification mode (allow self-signed for flexibility)
        SSL_CTX_set_verify(sslCtx, SSL_VERIFY_NONE, nullptr);

        // Create SSL connection
        ssl = SSL_new(sslCtx);
        if (!ssl) {
            SSL_CTX_free(sslCtx);
            close(sock);
            return nova_response_error();
        }

        SSL_set_fd(ssl, sock);
        SSL_set_tlsext_host_name(ssl, host.c_str());  // SNI

        if (SSL_connect(ssl) <= 0) {
            SSL_free(ssl);
            SSL_CTX_free(sslCtx);
            close(sock);
            return nova_response_error();
        }
    }
#else
    if (isHttps) {
        // HTTPS requires OpenSSL - compile with -DNOVA_HAS_OPENSSL
        close(sock);
        fprintf(stderr, "HTTPS not available: compile with OpenSSL support (-DNOVA_HAS_OPENSSL)\n");
        return nova_response_error();
    }
#endif

    // Build request
    std::ostringstream request;
    request << (method ? method : "GET") << " " << path << " HTTP/1.1\r\n";
    request << "Host: " << host << "\r\n";
    request << "User-Agent: Nova/1.0\r\n";

    if (headers) {
        for (const auto& pair : *headers->headers) {
            request << pair.first << ": " << pair.second << "\r\n";
        }
    }

    if (body) {
        request << "Content-Length: " << strlen(body) << "\r\n";
    }

    request << "Connection: close\r\n\r\n";

    if (body) {
        request << body;
    }

    std::string reqStr = request.str();

#ifdef NOVA_HAS_OPENSSL
    if (isHttps && ssl) {
        SSL_write(ssl, reqStr.c_str(), reqStr.length());
    } else {
        send(sock, reqStr.c_str(), reqStr.length(), 0);
    }
#else
    send(sock, reqStr.c_str(), reqStr.length(), 0);
#endif

    // Read response
    std::string response;
    char buffer[4096];
    ssize_t n;

#ifdef NOVA_HAS_OPENSSL
    if (isHttps && ssl) {
        while ((n = SSL_read(ssl, buffer, sizeof(buffer) - 1)) > 0) {
            buffer[n] = '\0';
            response += buffer;
        }
        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(sslCtx);
    } else {
        while ((n = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[n] = '\0';
            response += buffer;
        }
    }
#else
    while ((n = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[n] = '\0';
        response += buffer;
    }
#endif

    close(sock);

    // Parse response
    size_t headerEnd = response.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return nova_response_error();
    }

    std::string headerPart = response.substr(0, headerEnd);
    std::string bodyPart = response.substr(headerEnd + 4);

    // Parse status line
    int statusCode = 200;
    size_t firstSpace = headerPart.find(' ');
    if (firstSpace != std::string::npos) {
        size_t secondSpace = headerPart.find(' ', firstSpace + 1);
        if (secondSpace != std::string::npos) {
            statusCode = std::stoi(headerPart.substr(firstSpace + 1, secondSpace - firstSpace - 1));
        }
    }

    NovaResponse* res = static_cast<NovaResponse*>(
        nova_response_create(bodyPart.c_str(), statusCode, ""));

    free(res->url);
    res->url = strdup(url);

    return res;
}

#endif

// Main fetch function
void* nova_fetch(const char* url) {
    return doFetch(url, "GET", nullptr, nullptr);
}

void* nova_fetch_with_init(const char* url, const char* method, void* headersPtr, const char* body) {
    return doFetch(url, method, static_cast<NovaHeaders*>(headersPtr), body);
}

void* nova_fetch_request(void* reqPtr) {
    if (!reqPtr) return nova_response_error();

    NovaRequest* req = static_cast<NovaRequest*>(reqPtr);
    return doFetch(req->url, req->method, req->headers, req->body);
}

} // extern "C"
