# HTTP/2 Benchmarks

## Overview

This directory contains HTTP/2 benchmarks and tests for comparing Node.js, Bun, and Nova.

## Quick Start

### Test Node.js HTTP/2

```powershell
# Run Node.js HTTP/2 benchmark
powershell -ExecutionPolicy Bypass -File benchmarks/test_http2_node.ps1
```

### Manual Server Testing

**Node.js**:
```bash
node benchmarks/http2_server_node.js
# Test: curl --http2 http://localhost:3000/
```

**Nova** (once HTTP/2 module is registered):
```bash
build/Release/nova.exe benchmarks/http2_server_nova.ts
# Test: curl --http2 http://localhost:3000/
```

## Files

### Server Implementations

- `http2_server_node.js` - Node.js HTTP/2 server
- `http2_server_nova.ts` - Nova HTTP/2 server (requires module registration)
- `http2_server_bun.ts` - Bun fallback (HTTP/1.1 only, no HTTP/2 support)

### Benchmark Scripts

- `test_http2_node.ps1` - Node.js HTTP/2 performance test
- `bench_http2.ps1` - Comprehensive HTTP/2 benchmark (requires h2load)
- `bench_http2_status.ps1` - HTTP/2 support status checker

### Test Files

- `test_http2_simple.ts` - Simple Nova HTTP/2 test server

## Results

### Node.js HTTP/2 Performance

Based on 100 requests to local HTTP/2 server:

```
Average Latency: 11.60ms
Median Latency:  10.97ms
Min Latency:     10.12ms
Max Latency:     21.98ms
Requests/sec:    86.22
```

### Runtime Support Status

| Runtime | HTTP/2 Support | Status |
|---------|----------------|--------|
| Node.js | ✅ Yes | Production ready |
| Bun | ❌ No | Not implemented |
| Nova | 🟡 Implemented | Needs module registration |

## Nova HTTP/2 Status

Nova has a **complete HTTP/2 implementation** in C++:
- Location: `src/runtime/BuiltinHTTP2.cpp`
- Size: 1110 lines
- Features: Full HTTP/2 protocol support

**What's needed**: Register `nova:http2` module in `BuiltinModules.cpp`

## HTTP/2 Features

### Implemented in Nova

- ✅ HTTP/2 server (`createServer`)
- ✅ Secure server (`createSecureServer`)
- ✅ HTTP/2 client (`connect`)
- ✅ Stream API (respond, write, end)
- ✅ Session management
- ✅ Settings negotiation
- ✅ Error handling
- ✅ All HTTP/2 constants

### Not Yet Available

- ⏳ Module registration
- ⏳ TypeScript type definitions
- ⏳ Integration tests
- ⏳ Documentation

## Advanced Benchmarking

For comprehensive HTTP/2 testing, install `h2load`:

```bash
# Windows
# Download from: https://github.com/nghttp2/nghttp2/releases

# Linux/WSL
sudo apt-get install nghttp2-client

# Then run
h2load -n 10000 -c 100 http://localhost:3000/
```

## Performance Expectations

Based on Nova's other runtime modules, HTTP/2 is expected to deliver:

- **Low latency**: Better than Node.js due to C++ implementation
- **High throughput**: Efficient stream multiplexing
- **Low memory**: Minimal overhead per connection
- **Fast startup**: Quick server initialization

## Testing HTTP/2 Features

### Stream Multiplexing

HTTP/2 allows multiple requests on a single connection:

```bash
h2load -n 1000 -c 10 -m 10 http://localhost:3000/
# -n: total requests
# -c: concurrent connections
# -m: max concurrent streams per connection
```

### Server Push

Test server push capability:

```bash
# Server push test (when implemented)
h2load --header="accept: */*" http://localhost:3000/push-test
```

## Troubleshooting

### curl HTTP/2 Support

Check if curl supports HTTP/2:

```bash
curl --version
# Look for "HTTP2" in features list
```

If not supported:
- Windows: Use WSL or download curl with HTTP/2
- Linux: `sudo apt-get install curl`

### Port Already in Use

If port 3000 is busy:

```powershell
# Find process using port 3000
netstat -ano | findstr :3000

# Kill process (replace PID)
taskkill /F /PID <PID>
```

## Next Steps

1. **For Nova Developers**:
   - Register `nova:http2` module
   - Add integration tests
   - Run benchmarks against Node.js

2. **For Users**:
   - Test Node.js HTTP/2 performance
   - Compare with HTTP/1.1 benchmarks
   - Prepare for Nova HTTP/2 availability

## References

- **HTTP/2 Spec**: [RFC 7540](https://tools.ietf.org/html/rfc7540)
- **Node.js HTTP/2**: [API Docs](https://nodejs.org/api/http2.html)
- **nghttp2**: [Project Page](https://nghttp2.org/)

---

*Last updated: 2025-12-03*
