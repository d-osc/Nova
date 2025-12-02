// Simple benchmark for Nova - avoid deep recursion
console.log("=== Nova Benchmark Suite ===");

// 1. Loop performance
var loopStart = Date.now();
var sum = 0;
for (var i = 0; i < 10000000; i++) {
    sum += i;
}
var loopTime = Date.now() - loopStart;
console.log("Loop 10M iterations: " + loopTime + "ms");

// 2. Math operations
var mathStart = Date.now();
var result = 0;
for (var i = 0; i < 1000000; i++) {
    result = Math.sqrt(i) + Math.pow(i, 2) % 1000;
}
var mathTime = Date.now() - mathStart;
console.log("Math 1M operations: " + mathTime + "ms");

// 3. Array operations
var arrStart = Date.now();
var arr = [];
for (var i = 0; i < 100000; i++) {
    arr.push(i);
}
var arrTime = Date.now() - arrStart;
console.log("Array push 100K: " + arrTime + "ms");

// 4. String operations
var strStart = Date.now();
var str = "";
for (var i = 0; i < 10000; i++) {
    str = str + "a";
}
var strTime = Date.now() - strStart;
console.log("String concat 10K: " + strTime + "ms");

// 5. Object creation
var objStart = Date.now();
var objects = [];
for (var i = 0; i < 100000; i++) {
    objects.push({ id: i, value: i * 2 });
}
var objTime = Date.now() - objStart;
console.log("Object creation 100K: " + objTime + "ms");

// 6. Function calls
function add(a, b) {
    return a + b;
}
var funcStart = Date.now();
var funcResult = 0;
for (var i = 0; i < 1000000; i++) {
    funcResult = add(funcResult, 1);
}
var funcTime = Date.now() - funcStart;
console.log("Function calls 1M: " + funcTime + "ms");

// 7. Iterative fibonacci (no recursion)
function fibIterative(n) {
    if (n <= 1) return n;
    var a = 0;
    var b = 1;
    for (var i = 2; i <= n; i++) {
        var temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}
var fibStart = Date.now();
var fibResult = fibIterative(40);
var fibTime = Date.now() - fibStart;
console.log("Fibonacci(40) iterative: " + fibResult + " in " + fibTime + "ms");

// 8. Prime counting (iterative)
function countPrimes(max) {
    var count = 0;
    for (var n = 2; n <= max; n++) {
        var isPrime = true;
        for (var j = 2; j * j <= n; j++) {
            if (n % j === 0) {
                isPrime = false;
                break;
            }
        }
        if (isPrime) count++;
    }
    return count;
}
var primeStart = Date.now();
var primeCount = countPrimes(50000);
var primeTime = Date.now() - primeStart;
console.log("Primes to 50000: " + primeCount + " in " + primeTime + "ms");

// Total
var total = loopTime + mathTime + arrTime + strTime + objTime + funcTime + fibTime + primeTime;
console.log("");
console.log("Total: " + total + "ms");
