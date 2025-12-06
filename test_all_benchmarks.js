// Test all core benchmarks
console.log("=== Testing All Core Benchmarks ===\n");

var benchmarks = [
    "hello.js",
    "fib.js",
    "loop.js",
    "prime.js",
    "bench_array.js",
    "bench_array_simple.js",
    "bench_array_minimal.js",
    "bench_string.js",
    "bench_compute.js",
    "compute_bench.js",
    "bench_json.js",
    "bench_startup.js",
    "bench_minimal.js",
    "bench_memory.js",
    "bench_nova_simple.js",
    "bench_nova_stable.js",
    "test_hello.js"
];

console.log("Total benchmarks to test: " + benchmarks.length);
console.log("\nStarting tests...\n");

for (var i = 0; i < benchmarks.length; i++) {
    console.log((i + 1) + ". " + benchmarks[i]);
}

console.log("\n=== Test List Complete ===");
