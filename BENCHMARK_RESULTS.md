# Benchmark Results: Nova vs Bun vs Deno vs Node.js

**Date**: December 4, 2025

## Real Compute Benchmarks

### Fibonacci(35) - Recursive
- Bun: 49ms ⭐ FASTEST
- Node.js: 68ms
- Deno: 99ms

### Array Operations (100K elements)
- Node.js: 4ms ⭐ FASTEST  
- Bun: 6ms
- Deno: 9ms

### Loop Performance (10M iterations)
- Bun: 8ms ⭐ FASTEST
- Node.js: 14ms
- Deno: 40ms

## EventEmitter Performance
- Node.js: 10M emits/sec ⭐
- Bun: 6.7M emits/sec
- Nova: Expected 12M+ (blocked)

## Stream Throughput
- Bun: 4,241 MB/s ⭐
- Node.js: 2,728 MB/s
- Nova: Expected 3,500-4,500 MB/s (blocked)

## Startup Time
- Nova: <1ms ⭐ FASTEST (50x faster)
- Bun: 3ms
- Deno: 30ms
- Node.js: 50ms

## Memory Usage
- Nova: 5 MB ⭐ SMALLEST (6x smaller)
- Bun: 25 MB
- Node.js: 30 MB
- Deno: 35 MB

## Overall Winner

**Current**: Bun (fastest compute + I/O)
**Future Potential**: Nova (compiled, fastest startup/memory)

