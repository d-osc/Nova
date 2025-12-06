console.log("=== Testing bench_nova_simple sections ===");

// Section 1: Large loop (10M)
console.log("\n1. Testing 10M loop...");
var loopStart = Date.now();
var sum = 0;
for (var i = 0; i < 10000000; i++) {
    sum += i;
}
var loopTime = Date.now() - loopStart;
console.log("Loop 10M: " + loopTime + "ms, sum=" + sum);

console.log("\nSection 1 complete!");
