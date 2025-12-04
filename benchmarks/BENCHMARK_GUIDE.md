# Nova Performance Benchmark Guide

## Overview

This guide provides comprehensive benchmarking strategies to evaluate Nova's performance against Node.js and Bun across different workload types.

## Benchmark Categories

### 1. HTTP Throughput Benchmarks ⚠️ (In Progress)

**Location**: `benchmarks/bench_http_comprehensive.ps1`

**Measures**:
- **Requests/sec (RPS)**: Server throughput
- **Latency (p50/p90/p99)**: Response time distribution
- **CPU Usage**: Processor utilization percentage
- **Memory Usage**: RAM consumption (avg and peak)

**Test Scenarios**:
1. **Hello World** - Baseline HTTP performance
2. **Routing + JSON** - Realistic CRUD API operations

**Concurrency Levels**: 1, 50, 500 (adjustable)

**Status**: HTTP module needs completion. See `benchmarks/README_HTTP_BENCHMARKS.md` for details.

### 2. JSON Performance Benchmarks ✅ (Working)

**Location**: `benchmarks/json_perf_final.ts`

Tests JSON parsing and serialization performance - critical for REST APIs.

**Run with**:
```bash
# Compare all three runtimes
./build/Release/nova.exe benchmarks/json_perf_final.ts
node benchmarks/json_perf_final.ts
bun benchmarks/json_perf_final.ts
```

**What it tests**:
- JSON.parse() performance
- JSON.stringify() performance
- Complex object handling
- Array serialization

### 3. Compute Benchmarks ✅ (Working)

**Location**: `benchmarks/compute_bench.ts`

Tests raw computational performance.

**Run with**:
```bash
./build/Release/nova.exe benchmarks/compute_bench.ts
node benchmarks/compute_bench.js
bun benchmarks/compute_bench.ts
```

**What it tests**:
- Recursive functions (factorial)
- Loops and arithmetic
- Function call overhead

### 4. Startup Time Benchmarks ✅ (Working)

**Location**: `benchmarks/measure_startup.ps1`

Measures cold start performance - important for CLI tools and serverless.

**Run with**:
```powershell
powershell -ExecutionPolicy Bypass -File benchmarks/measure_startup.ps1
```

**What it tests**:
- Time to execute simple script
- Runtime initialization overhead
- JIT compilation time

## Recommended Benchmark Flow

### Quick Performance Check (5 minutes)

```powershell
# 1. Startup time
powershell -ExecutionPolicy Bypass -File benchmarks/measure_startup.ps1

# 2. Compute performance (run 10 times each)
for i in 1 2 3 4 5 6 7 8 9 10
do echo "Test $i:"
do powershell -Command "Measure-Command { ./build/Release/nova.exe benchmarks/compute_bench.ts 2>&1 | Out-Null } | Select-Object -ExpandProperty TotalMilliseconds"
done

# 3. JSON performance
./build/Release/nova.exe benchmarks/json_perf_final.ts
node benchmarks/json_perf_final.ts
bun benchmarks/json_perf_final.ts
```

### Comprehensive Performance Analysis (30+ minutes)

```powershell
# 1. All quick checks above

# 2. HTTP benchmarks (when ready)
powershell -ExecutionPolicy Bypass -File benchmarks/bench_http_comprehensive.ps1 -Duration 30 -Concurrency 1,10,50,100,500

# 3. Memory profiling
# Run extended tests and monitor with Task Manager or perfmon
```

## Performance Metrics Guide

### Primary Metrics

| Metric | Good | Warning | Poor | Notes |
|--------|------|---------|------|-------|
| **Startup** | <50ms | 50-200ms | >200ms | Cold start time |
| **RPS (c=1)** | >5000 | 1000-5000 | <1000 | Single thread perf |
| **RPS (c=50)** | >20000 | 5000-20000 | <5000 | Concurrent perf |
| **p50 latency** | <5ms | 5-20ms | >20ms | Median response |
| **p99 latency** | <50ms | 50-200ms | >200ms | Tail latency |
| **Memory (idle)** | <30MB | 30-100MB | >100MB | Base memory |
| **CPU efficiency** | High RPS/CPU% | - | Low RPS/CPU% | Work per CPU |

### Efficiency Ratios

Calculate these to compare across runtimes:

```
RPS per CPU% = Total RPS / CPU Percentage
RPS per MB = Total RPS / Memory MB
Overall Efficiency = RPS / (CPU% + Memory_MB)
```

Higher values = better efficiency

## Use Case Performance Priorities

### CLI Tools / Scripts
1. **Startup time** (most critical)
2. Compute performance
3. Memory usage

**Target**: <50ms startup, <50MB memory

### REST APIs / Web Services
1. **RPS and latency** (most critical)
2. CPU efficiency
3. Memory scaling under load

**Target**: >10k RPS @ c=50, p99 <50ms

### Data Processing
1. **Compute performance** (most critical)
2. JSON/serialization speed
3. Memory efficiency

**Target**: High throughput, stable memory

### Real-time Services
1. **p99 latency** (most critical)
2. Consistent performance
3. Memory stability

**Target**: p99 <20ms, no spikes

## Benchmark Comparison Template

```
=================================================
NOVA vs NODE vs BUN - Performance Comparison
=================================================

Test Environment:
- OS: Windows 11
- CPU: [Your CPU]
- RAM: [Your RAM]
- Nova: [git commit]
- Node: [version]
- Bun: [version]

Startup Time (10 runs avg):
Nova: ____ ms
Node: ____ ms
Bun: ____ ms
Winner: ____

Compute Benchmark (10 runs avg):
Nova: ____ ms
Node: ____ ms
Bun: ____ ms
Winner: ____

JSON Performance:
Parse: Nova ____ vs Node ____ vs Bun ____
Stringify: Nova ____ vs Node ____ vs Bun ____
Winner: ____

HTTP Hello World (c=50):
Nova: ____ RPS, p99 ____ ms
Node: ____ RPS, p99 ____ ms
Bun: ____ RPS, p99 ____ ms
Winner: ____

Memory Usage (HTTP c=50):
Nova: ____ MB avg, ____ MB peak
Node: ____ MB avg, ____ MB peak
Bun: ____ MB avg, ____ MB peak
Winner: ____

Overall Assessment:
- Best for CLI: ____
- Best for APIs: ____
- Best for compute: ____
- Most efficient: ____
```

## Tips for Accurate Benchmarks

### Before Testing
1. Close unnecessary applications
2. Disable antivirus scanning temporarily
3. Ensure AC power (not battery)
4. Let system idle for 1 minute

### During Testing
1. Run each test multiple times (5-10)
2. Discard first run (warmup)
3. Calculate average and standard deviation
4. Watch for outliers

### Analyzing Results
1. Compare like-for-like (same concurrency, duration)
2. Consider both absolute and relative performance
3. Look at performance under load, not just peak
4. Evaluate consistency (std deviation)
5. Consider resource efficiency, not just speed

## Automated Benchmark Suite

For continuous integration, create a script:

```powershell
# benchmark_all.ps1
param([string]$OutputFile = "benchmark_results.json")

Write-Host "Running full benchmark suite..."

$results = @{}

# Startup
$results.startup = @{
    nova = # measure
    node = # measure
    bun = # measure
}

# Compute
# ... etc

$results | ConvertTo-Json -Depth 10 | Out-File $OutputFile
Write-Host "Results saved to $OutputFile"
```

## Known Issues & Limitations

### Current Limitations
1. **HTTP benchmarks require completed HTTP module** - Property binding in progress
2. **Database benchmarks not yet implemented** - Postgres/Redis integration pending
3. **Windows-only scripts** - Need cross-platform versions

### Planned Additions
- [ ] Database (Postgres/Redis) benchmarks
- [ ] WebSocket performance tests
- [ ] File I/O benchmarks
- [ ] Crypto/hashing benchmarks
- [ ] HTTP/2 and HTTP/3 tests

## Getting Help

- **Benchmark issues**: Check `benchmarks/README_HTTP_BENCHMARKS.md`
- **HTTP module status**: See `src/runtime/BuiltinHTTP.cpp`
- **Results interpretation**: See "Performance Metrics Guide" above

## Contributing Benchmarks

To add new benchmarks:

1. Create test files for Nova/Node/Bun
2. Add runner script (PowerShell and/or Node.js)
3. Document expected results
4. Add to this guide
5. Create PR with benchmark results

Target:
- Representative of real-world usage
- Reproducible across systems
- Fast enough to run frequently (<5 min)
- Clear success criteria
