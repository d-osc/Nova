# HTTP Performance Benchmarks

This directory contains comprehensive HTTP performance benchmarks to compare Nova against Node.js and Bun.

## Benchmark Files

### Hello World Servers
Simple HTTP servers that respond with "Hello World":
- `http_hello_nova.ts` - Nova implementation
- `http_hello_node.js` - Node.js implementation
- `http_hello_bun.ts` - Bun implementation

### Routing Servers
HTTP servers with CRUD endpoints for user management:
- `http_routing_nova.ts` - Nova implementation with routes: GET /, GET /users, GET /users/:id, POST /users, PUT /users/:id, DELETE /users/:id
- `http_routing_node.js` - Node.js implementation
- `http_routing_bun.ts` - Bun implementation

### Benchmark Runners

#### PowerShell Runner (Recommended for Windows)
`bench_http_comprehensive.ps1` - Comprehensive benchmark script that measures:
- **Throughput**: Requests per second (RPS)
- **Latency**: p50, p90, p99 response times
- **CPU Usage**: CPU percentage during load
- **Memory Usage**: Average and max memory in MB

**Usage:**
```powershell
# Run full benchmark suite (10s per test, concurrency: 1, 50, 500)
powershell -ExecutionPolicy Bypass -File benchmarks/bench_http_comprehensive.ps1

# Custom duration and concurrency levels
powershell -ExecutionPolicy Bypass -File benchmarks/bench_http_comprehensive.ps1 -Duration 30 -Warmup 5 -Concurrency 1,10,50,100,500

# Quick test
powershell -ExecutionPolicy Bypass -File benchmarks/bench_http_comprehensive.ps1 -Duration 5 -Warmup 1 -Concurrency 10,50
```

**Parameters:**
- `-Duration` - Test duration in seconds (default: 10)
- `-Warmup` - Warmup duration in seconds (default: 2)
- `-Concurrency` - Array of concurrency levels to test (default: 1, 50, 500)

#### Node.js Runner (Cross-platform)
`http_bench_runner.js` - Alternative Node.js-based benchmark runner

**Usage:**
```bash
node benchmarks/http_bench_runner.js
```

## Benchmark Scenarios

### 1. Hello World (Baseline)
Tests raw HTTP throughput with minimal processing:
- Measures: RPS, latency, CPU, memory
- Endpoints: GET /
- Expected performance order: Bun > Nova > Node

### 2. Routing with JSON (Realistic)
Tests routing, JSON parsing/serialization, and in-memory operations:
- Measures: RPS, latency, CPU, memory under realistic load
- Endpoints: Multiple CRUD operations
- Tests realistic application scenarios

## Metrics Explained

### Throughput (RPS)
Requests per second the server can handle. Higher is better.
- **Concurrency 1**: Sequential request performance
- **Concurrency 50**: Typical web application load
- **Concurrency 500**: High load scenario

### Latency Percentiles
Response time distribution under load:
- **p50 (median)**: 50% of requests complete within this time
- **p90**: 90% of requests complete within this time
- **p99**: 99% of requests complete within this time (tail latency)

Lower values are better. p99 is critical for user experience.

### CPU Usage
CPU percentage used during the benchmark. Lower is better for same throughput.

### Memory Usage
RAM consumption in MB. Lower is better for same throughput.
- **Avg Memory**: Average during test
- **Max Memory**: Peak memory usage

## Performance Analysis Tips

### Throughput vs Resources
Compare RPS per CPU% and RPS per MB memory:
```
Efficiency = RPS / (CPU% + Memory_MB)
```

### Latency Under Load
Good performance means:
- p50 stays low even at high concurrency
- p99 doesn't spike dramatically
- Consistent performance across concurrency levels

### Expected Results
Typical performance characteristics:
- **Bun**: Highest RPS, but may use more memory
- **Nova**: Competitive RPS with lower memory (when C++ backend fully optimized)
- **Node**: Baseline performance, higher latency under load

## Current Status

⚠️ **Note**: Nova's HTTP module implementation is partially complete. The C++ runtime functions exist in `src/runtime/BuiltinHTTP.cpp`, but need full integration with the type system to expose methods like `writeHead()` and `end()` to TypeScript code.

**To enable these benchmarks:**
1. Complete HTTP module type bindings in HIRGen
2. Ensure ServerResponse and IncomingMessage types are properly exposed
3. Test with: `./build/Release/nova.exe benchmarks/http_hello_nova.ts`

## Alternative: Compute Benchmarks

While HTTP benchmarks are being completed, you can use compute benchmarks:
- `compute_bench.ts` - Tests raw computation performance
- `json_perf_final.ts` - Tests JSON parsing/serialization performance

Run with:
```bash
./build/Release/nova.exe benchmarks/compute_bench.ts
node benchmarks/compute_bench.js
bun benchmarks/compute_bench.ts
```

## Example Output

```
======================================================================
Benchmark: Hello World | Runtime: Nova | Concurrency: 50
======================================================================
Starting Nova server...
Nova server started (PID: 12345)
Warming up for 2 seconds...
Running benchmark for 10 seconds...

--- Results ---
Total Requests: 45230
Successful: 45230
RPS: 4523.00 req/sec

Latency:
  Min: 0.45 ms
  Avg: 11.05 ms
  p50: 10.23 ms
  p90: 15.67 ms
  p99: 23.45 ms
  Max: 45.12 ms

Resources:
  Avg Memory: 24.50 MB
  Max Memory: 28.30 MB
  CPU: 45.20%
```

## Contributing

To add new benchmark scenarios:
1. Create server implementations for Nova, Node, and Bun
2. Add configuration to the benchmark runner
3. Document expected performance characteristics
4. Update this README
