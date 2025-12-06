console.log("=== Testing remaining sections ===");

// 5. Object creation
console.log("Testing object creation...");
var objects = [];
for (var i = 0; i < 100000; i++) {
    objects.push({ id: i, value: i * 2 });
}
console.log("5. Object creation OK: " + objects.length);

// 6. Function calls
console.log("Testing function calls...");
function add(a, b) {
    return a + b;
}
var funcResult = 0;
for (var i = 0; i < 1000000; i++) {
    funcResult = add(funcResult, 1);
}
console.log("6. Function calls OK: " + funcResult);

// 7. Fibonacci iterative
console.log("Testing fibonacci...");
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
var fibResult = fibIterative(40);
console.log("7. Fibonacci OK: " + fibResult);

// 8. Prime counting
console.log("Testing prime counting...");
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
var primeCount = countPrimes(50000);
console.log("8. Prime counting OK: " + primeCount);

console.log("\n=== All remaining sections passed! ===");
