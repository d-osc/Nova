// Test 3: Three function definitions (like in failing test)
console.log("Test 3: Three functions");
function add(a, b) {
    return a + b;
}
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
console.log("Testing add: " + add(1, 2));
console.log("Testing fib: " + fibIterative(10));
console.log("Testing primes: " + countPrimes(100));
