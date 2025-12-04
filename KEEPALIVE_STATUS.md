# Nova HTTP Keep-Alive Implementation Status

**Date:** December 3, 2025
**Status:** Phase 1 Complete (Detection) - Phase 2 Needed (Reuse)

---

## สิ่งที่ทำเสร็จแล้ว ✅ (Phase 1)

### 1. Keep-Alive Detection Logic

**ไฟล์:** `src/runtime/BuiltinHTTP.cpp` (Lines 1285-1322, 1420-1457)

**Features Implemented:**

#### A. HTTP Version Detection
```cpp
bool isHttp11 = (req->httpVersion && strcmp(req->httpVersion, "1.1") == 0);
```
- ตรวจจับว่าเป็น HTTP/1.0 หรือ HTTP/1.1

#### B. Connection Header Parsing
```cpp
char* connectionHeader = nova_http_IncomingMessage_getHeader(req, "connection");
std::string conn = connectionHeader;
// Convert to lowercase for case-insensitive comparison
for (auto& c : conn) c = (char)tolower(c);
```
- อ่าน `Connection:` header
- Case-insensitive comparison

#### C. Keep-Alive Decision Logic
```cpp
if (isHttp11) {
    // HTTP/1.1: keep-alive by default unless "close" specified
    keepAlive = (conn.find("close") == std::string::npos);
} else {
    // HTTP/1.0: close by default unless "keep-alive" specified
    keepAlive = (conn.find("keep-alive") != std::string::npos);
}
```

**Follows RFC 2616:**
- HTTP/1.1: Keep-Alive เป็น default (ปิดก็ต่อเมื่อมี `Connection: close`)
- HTTP/1.0: Close เป็น default (เปิดก็ต่อเมื่อมี `Connection: keep-alive`)

### 2. Cross-Platform Support

✅ **Windows** (Lines 1285-1322)
✅ **POSIX** (Linux/Mac) (Lines 1420-1457)

ทั้งสอง platforms มี logic เดียวกัน

---

## สิ่งที่ยังไม่ได้ทำ ⚠️ (Phase 2)

### ปัญหา: ยังปิด Socket อยู่!

```cpp
// Lines 1319-1322 and 1453-1457
// Keep-Alive: Set socket back to blocking mode and continue handling requests
// For now, we'll close the connection to maintain compatibility
// TODO: Implement proper keep-alive with connection pooling
CLOSE_SOCKET(clientSocket);
return 1;
```

**ปัจจุบัน:** ตรวจจับได้ว่าควร keep-alive ไหม แต่ยังปิด socket ทุกครั้ง!
**ผลลัพธ์:** ยังไม่ได้ throughput improvement

---

## Phase 2: Full Keep-Alive Implementation

### สิ่งที่ต้องทำ:

#### 1. **Connection Reuse Loop**

แทนที่จะ return ทันที ต้องวนอ่าน request ต่อบน socket เดิม:

```cpp
// Pseudocode
while (keepAlive) {
    // Wait for next request (with timeout)
    int bytesRead = recv_with_timeout(clientSocket, buffer, timeout=5000ms);

    if (bytesRead <= 0) {
        // Timeout or client closed
        break;
    }

    // Parse and handle request
    parseAndHandleRequest();

    // Check if client wants to keep connection alive
    keepAlive = shouldKeepAlive(req);
}

CLOSE_SOCKET(clientSocket);
```

#### 2. **Timeout Handling**

```cpp
// Set socket timeout for idle connections
struct timeval timeout;
timeout.tv_sec = 5;   // 5 seconds idle timeout
timeout.tv_usec = 0;
setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
```

#### 3. **Request Pipelining Support**

HTTP/1.1 clients อาจส่ง multiple requests ใน 1 connection:
```
GET /page1 HTTP/1.1
Host: example.com

GET /page2 HTTP/1.1
Host: example.com
```

ต้อง handle buffer ที่มี partial requests

#### 4. **Connection Pool Management**

สำหรับ scalability:
- Track active connections
- Limit max connections per server
- Close oldest idle connections when limit reached

---

## Architecture Options

### Option A: Simple Loop (Easiest)

```cpp
int nova_http_Server_acceptOne(...) {
    // Accept connection
    socket_t clientSocket = accept(server->socket, ...);

    // Set timeout for keep-alive
    setTimeout(clientSocket, 5000ms);

    // Process multiple requests on same connection
    while (true) {
        // Read request
        int bytesRead = recv(clientSocket, buffer, ...);
        if (bytesRead <= 0) break;  // Timeout or close

        // Parse and handle
        IncomingMessage* req = parseRequest(buffer);
        ServerResponse* res = createResponse(clientSocket);
        server->onRequest(req, res);

        // Check keep-alive
        bool keepAlive = shouldKeepAlive(req);
        cleanup(req, res);

        if (!keepAlive) break;
    }

    CLOSE_SOCKET(clientSocket);
    return 1;
}
```

**Pros:**
- Simple to implement (2-4 hours)
- Works for sequential requests on each connection

**Cons:**
- Blocks while handling one connection
- Can't handle concurrent connections efficiently

---

### Option B: Event Loop with select/epoll (Best)

```cpp
// Maintain list of active connections
std::vector<Connection*> activeConnections;

int nova_http_Server_run(...) {
    while (running) {
        // Use select() to wait for activity on any socket
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server->socket, &readfds);  // Listen socket

        for (auto conn : activeConnections) {
            FD_SET(conn->socket, &readfds);  // Client sockets
        }

        int activity = select(max_fd + 1, &readfds, ...);

        // Check for new connection
        if (FD_ISSET(server->socket, &readfds)) {
            socket_t newSocket = accept(...);
            activeConnections.push_back(new Connection(newSocket));
        }

        // Check existing connections
        for (auto conn : activeConnections) {
            if (FD_ISSET(conn->socket, &readfds)) {
                handleRequest(conn);

                if (!conn->keepAlive || conn->timedOut) {
                    closeAndRemove(conn);
                }
            }
        }
    }
}
```

**Pros:**
- Can handle many concurrent connections
- True asynchronous I/O
- Maximum throughput potential

**Cons:**
- More complex (1-2 days)
- Requires significant architecture changes

---

## Recommended Approach

### Step 1: Implement Option A (Simple Loop) ⚡

**Timeline:** 2-4 hours
**Expected Gain:** **2-5x throughput**

**Why:**
- Quick win
- Minimal code changes
- Maintains current architecture
- Good enough for many use cases

### Step 2: Later implement Option B (Event Loop) 🚀

**Timeline:** 1-2 days
**Expected Gain:** **10-50x throughput**

**Why:**
- True scalability
- Handle thousands of concurrent connections
- Competitive with best-in-class servers

---

## Testing Strategy

### Test Keep-Alive Detection (Already Works!)

```bash
# Test with curl (HTTP/1.1 default keep-alive)
curl -v http://localhost:3000/

# Should see in response headers:
# Connection: keep-alive (or nothing, which means keep-alive in HTTP/1.1)

# Test with explicit Connection: close
curl -v -H "Connection: close" http://localhost:3000/

# Test with HTTP/1.0
curl -v --http1.0 http://localhost:3000/
```

### Test Connection Reuse (After Phase 2)

```bash
# Use curl with --keepalive-time
curl -v --keepalive-time 60 http://localhost:3000/page1 http://localhost:3000/page2

# Should reuse same TCP connection for both requests
```

### Benchmark with ab/wrk

```bash
# ApacheBench with keep-alive
ab -k -n 10000 -c 100 http://localhost:3000/

# wrk (uses keep-alive by default)
wrk -t4 -c100 -d30s http://localhost:3000/
```

---

## Performance Projection

### Current (No Keep-Alive):
```
Sequential: 8.26 req/sec
Concurrent (ab -c100): ~100-200 req/sec (estimated)
```

### After Phase 1 (Detection Only):
```
No change - still closing sockets
```

### After Phase 2 Option A (Simple Loop):
```
Concurrent (ab -k -c100): ~500-1000 req/sec
Gain: 2-5x
Competitive with Node.js
```

### After Phase 2 Option B (Event Loop):
```
Concurrent (ab -k -c100): ~2000-5000 req/sec
Gain: 10-25x
Potentially faster than Node.js!
```

---

## Next Steps

### Immediate (Today):

1. ✅ Phase 1 Complete - Keep-alive detection implemented
2. ⏭️ **Implement Phase 2 Option A** - Simple keep-alive loop
   - Estimated time: 2-4 hours
   - Expected gain: 2-5x throughput
3. ⏭️ Test with ab -k
4. ⏭️ Benchmark vs Node/Bun

### Future (This Week):

5. Implement Phase 2 Option B - Event loop architecture
6. Connection pool management
7. Load testing with 1000+ concurrent connections

---

## Code Changes Needed for Phase 2 Option A

### File: `src/runtime/BuiltinHTTP.cpp`

**Location:** Lines 1319-1322 (Windows) and 1453-1457 (POSIX)

**Replace:**
```cpp
CLOSE_SOCKET(clientSocket);
return 1;
```

**With:**
```cpp
if (!keepAlive) {
    CLOSE_SOCKET(clientSocket);
    return 1;
}

// Keep-Alive: Process additional requests on same connection
// Set socket timeout (5 seconds for idle connections)
#ifdef _WIN32
DWORD timeout = 5000;  // 5 seconds in milliseconds
setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
#else
struct timeval tv;
tv.tv_sec = 5;
tv.tv_usec = 0;
setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif

// Loop to handle multiple requests on this connection
while (keepAlive) {
    // Read next request
    char buffer[8192];
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        // Timeout or client closed connection
        break;
    }

    buffer[bytesRead] = '\0';

    // Parse request
    IncomingMessage* req = (IncomingMessage*)nova_http_IncomingMessage_new();
    req->socket = clientSocket;

    if (!parseHttpRequest(buffer, req)) {
        nova_http_IncomingMessage_free(req);
        break;
    }

    // Create response
    ServerResponse* res = (ServerResponse*)nova_http_ServerResponse_new(clientSocket);

    // Call request handler
    if (server->onRequest) {
        server->onRequest(req, res);
    }

    // Ensure response is sent
    if (!res->finished) {
        nova_http_ServerResponse_end(res, nullptr, 0);
    }

    // Check if we should keep connection alive for next request
    bool isHttp11 = (req->httpVersion && strcmp(req->httpVersion, "1.1") == 0);
    char* connectionHeader = nova_http_IncomingMessage_getHeader(req, "connection");

    if (connectionHeader) {
        std::string conn = connectionHeader;
        for (auto& c : conn) c = (char)tolower(c);

        if (isHttp11) {
            keepAlive = (conn.find("close") == std::string::npos);
        } else {
            keepAlive = (conn.find("keep-alive") != std::string::npos);
        }
        free(connectionHeader);
    } else {
        keepAlive = isHttp11;
    }

    // Clean up
    nova_http_IncomingMessage_free(req);
    nova_http_ServerResponse_free(res);
}

// Close connection after keep-alive loop ends
CLOSE_SOCKET(clientSocket);
return 1;
```

**Estimated Lines:** ~60 lines of code
**Complexity:** Medium
**Time:** 2-4 hours with testing

---

## Summary

**Phase 1:** ✅ Keep-Alive Detection - COMPLETE
**Phase 2:** ⏭️ Keep-Alive Reuse - READY TO IMPLEMENT

**Next Action:** Implement the code changes above to get **2-5x throughput improvement**!

---

*Document Created: December 3, 2025*
*Status: Detection implemented, reuse pending*
*Expected Time to Complete: 2-4 hours*
*Expected Performance Gain: 2-5x throughput*
