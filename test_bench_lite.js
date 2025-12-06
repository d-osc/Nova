console.log("=== Nova Benchmark Suite (Lite) ===");

// 1. Loop performance
var loopStart = Date.now();
var sum = 0;
for (var i = 0; i < 100000; i++) {  // Reduced from 10M
    sum += i;
}
var loopTime = Date.now() - loopStart;
console.log("Loop 100K iterations: " + loopTime + "ms");

// 2. Math operations
var mathStart = Date.now();
var result = 0;
for (var i = 0; i < 10000; i++) {  // Reduced from 1M
    result = Math.sqrt(i) + Math.pow(i, 2) % 1000;
}
var mathTime = Date.now() - mathStart;
console.log("Math 10K operations: " + mathTime + "ms");

// 3. Array operations
var arrStart = Date.now();
var arr = [];
for (var i = 0; i < 10000; i++) {  // Reduced from 100K
    arr.push(i);
}
var arrTime = Date.now() - arrStart;
console.log("Array push 10K: " + arrTime + "ms");

// Total
var total = loopTime + mathTime + arrTime;
console.log("");
console.log("Total: " + total + "ms");
