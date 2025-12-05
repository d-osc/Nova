# Nova HTTP Throughput Optimization Plan

**Goal:** ทำให้ Nova HTTP throughput สูงที่สุดเท่าที่เป็นไปได้

---

## ปัญหาที่เจอ (Bottlenecks Identified)

### 1. **ปิด Connection ทุกครั้ง** ❌ (Impact สูงสุด!)

**Problem:**
```cpp
// Line 1407 in BuiltinHTTP.cpp
CLOSE_SOCKET(clientSocket);  // ← ปิดทุก request!
```

**Impact:**
- ต้องทำ TCP 3-way handshake ใหม่ทุกครั้ง (~3 RTT overhead)
- ไม่มี connection reuse
- Throughput ลดลงมาก

**Solution:**
- เปิดใช้ HTTP/1.1 Keep-Alive
- เก็บ connection ไว้รอ request ถัดไป
- ปิดเมื่อ client ส่ง `Connection: close` หรือ timeout

**Expected Improvement:** **2-5x throughput**

---

### 2. **Blocking I/O** ⚠️ (Impact ปานกลาง)

**Problem:**
```cpp
// Line 1353
int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
```

- ใช้ blocking recv() แบบ synchronous
- ไม่สามารถ handle concurrent connections พร้อมกันได้

**Solution:**
- เปลี่ยนเป็น non-blocking I/O + select/poll
- หรือใช้ thread pool สำหรับ concurrent connections

**Expected Improvement:** **3-10x throughput** (for concurrent load)

---

### 3. **Memory Allocation ทุก Request** ⚠️ (Impact น้อย)

**Problem:**
```cpp
// Lines 1363, 1373
IncomingMessage* req = new IncomingMessage();
ServerResponse* res = new ServerResponse(clientSocket);
```

- Allocate/deallocate objects ทุก request
- Overhead จาก memory allocation

**Solution:**
- Object pooling - reuse objects
- Pre-allocate buffers

**Expected Improvement:** **10-20% throughput**

---

### 4. **Parsing Overhead** ⚠️ (Impact น้อย)

**Problem:**
- Parse HTTP headers ทุกครั้งแม้จะเป็น header เดิม
- String operations ช้า

**Solution:**
- Cache parsed results
- Optimize string parsing with SIMD
- Use zero-copy techniques

**Expected Improvement:** **5-15% throughput**

---

## Optimizations แบ่งตาม Priority

### 🔥 Priority 1: Enable Keep-Alive (Must Have!)

**Implementation:**
```cpp
// ใน acceptOne function
bool keepAlive = true;  // HTTP/1.1 default is keep-alive
char* connectionHeader = req->getHeader("connection");
if (connectionHeader) {
    keepAlive = (strcasecmp(connectionHeader, "close") != 0);
}

if (keepAlive) {
    // Don't close socket, loop back to read next request
    continue_reading_on_same_socket();
} else {
    CLOSE_SOCKET(clientSocket);
}
```

**Estimated Time:** 2-4 hours
**Expected Gain:** **2-5x throughput**

---

### 🔥 Priority 2: Non-Blocking I/O + Event Loop

**Implementation:**
- ใช้ `select()` หรือ `epoll` (Linux) / `IOCP` (Windows)
- Multiple connections in event loop
- Non-blocking sockets

**Estimated Time:** 1-2 days
**Expected Gain:** **3-10x throughput** (concurrent)

---

### ⚡ Priority 3: Object Pooling

**Implementation:**
```cpp
// Object pool
std::vector<IncomingMessage*> reqPool;
std::vector<ServerResponse*> resPool;

IncomingMessage* getReq() {
    if (reqPool.empty()) return new IncomingMessage();
    auto req = reqPool.back();
    reqPool.pop_back();
    req->reset();
    return req;
}

void returnReq(IncomingMessage* req) {
    reqPool.push_back(req);
}
```

**Estimated Time:** 2-4 hours
**Expected Gain:** **10-20% throughput**

---

### ⚡ Priority 4: Optimize Parsing

**Implementation:**
- Use faster string parsing
- Avoid unnecessary allocations
- Use string_view instead of string copies

**Estimated Time:** 4-8 hours
**Expected Gain:** **5-15% throughput**

---

## การทดสอบที่เหมาะสม

### เครื่องมือแนะนำ:

1. **Apache Bench (ab)**
```bash
ab -n 10000 -c 100 http://localhost:3000/
```

2. **wrk** (ดีกว่า ab)
```bash
wrk -t4 -c100 -d30s http://localhost:3000/
```

3. **hey** (Go-based)
```bash
hey -n 10000 -c 100 http://localhost:3000/
```

### Test Cases:

1. **Low Concurrency** (c=1): วัด latency และ single-thread performance
2. **Medium Concurrency** (c=50): วัด typical load
3. **High Concurrency** (c=500): วัด maximum capacity
4. **Sustained Load** (30-60s): วัด memory leaks และ stability

---

## ผลลัพธ์ที่คาดหวัง

### ตอนนี้ (Before Optimization):
```
Sequential Test (Python client):
- Throughput: 8.26 req/sec
- Bottleneck: Client sending requests one-by-one

Concurrent Test (expected with ab -c100):
- Throughput: ~100-200 req/sec (ประมาณ, ยังไม่ได้ test)
- Bottleneck: Close socket every request
```

### หลัง Priority 1 (Keep-Alive):
```
Concurrent Test (ab -c100):
- Throughput: ~500-1000 req/sec
- Gain: 2-5x
- Competitive with Node.js
```

### หลัง Priority 2 (Non-Blocking I/O):
```
Concurrent Test (ab -c100):
- Throughput: ~2000-5000 req/sec
- Gain: 10-25x from baseline
- Competitive with Bun
```

### หลัง Priority 3+4 (Object Pool + Parsing):
```
Concurrent Test (ab -c100):
- Throughput: ~3000-8000 req/sec
- Gain: 15-40x from baseline
- Potentially faster than Node.js
```

---

## สรุปและ Next Steps

### ทำตอนนี้ได้เลย (Quick Wins):

1. **Test with `ab` or `wrk`** - รู้ throughput จริงๆ ตอนนี้
2. **Implement Keep-Alive** - ได้ 2-5x ทันที
3. **Re-test and compare** - เทียบกับ Node/Bun

### ทำในอนาคต (Bigger Wins):

4. **Non-blocking I/O** - Scale to thousands of concurrent connections
5. **Object pooling** - Reduce GC-like overhead
6. **SIMD parsing** - Ultra-fast header parsing

---

## Realistic Target

**Short-term (1-2 days):**
- Nova: 500-1000 req/sec (with keep-alive)
- Node: 500-800 req/sec
- Bun: 800-1200 req/sec

**Result:** Nova competitive with Node, slightly behind Bun

**Medium-term (1 week):**
- Nova: 2000-5000 req/sec (with non-blocking I/O)
- Node: 800-1500 req/sec
- Bun: 1200-2000 req/sec

**Result:** Nova potentially faster than both!

---

## Key Insight

**ปัจจุบัน**: Nova infrastructure ดี (compiler, runtime) แต่ยังไม่ optimize สำหรับ throughput

**อนาคต**: Nova มี potential สูงเพราะ:
- Native compilation (no JIT overhead)
- No garbage collection pauses
- LLVM optimizations
- Direct syscall access

**With proper optimizations, Nova can beat Node.js and compete with Bun!** 🚀

---

*Document Created: December 3, 2025*
*Status: Bottlenecks identified, optimizations planned*
*Next Action: Implement Keep-Alive (2-4 hours, 2-5x gain)*
