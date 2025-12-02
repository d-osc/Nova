// Minimal benchmark
console.log("Starting...");

var start = Date.now();
var sum = 0;
for (var i = 0; i < 1000000; i++) {
    sum = sum + i;
}
var elapsed = Date.now() - start;

console.log("Sum: " + sum);
console.log("Time: " + elapsed + "ms");
