// Computation Benchmark - Fibonacci & Prime Numbers
const iterations = 40;

function fibonacci(n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

function isPrime(n) {
    if (n < 2) return false;
    for (let i = 2; i * i <= n; i++) {
        if (n % i === 0) return false;
    }
    return true;
}

function countPrimes(max) {
    let count = 0;
    for (let i = 2; i <= max; i++) {
        if (isPrime(i)) count++;
    }
    return count;
}

// Fibonacci benchmark
const fibStart = Date.now();
const fibResult = fibonacci(iterations);
const fibTime = Date.now() - fibStart;

// Prime counting benchmark
const primeStart = Date.now();
const primeCount = countPrimes(100000);
const primeTime = Date.now() - primeStart;

console.log(`Fibonacci(${iterations}): ${fibResult} in ${fibTime}ms`);
console.log(`Primes up to 100000: ${primeCount} in ${primeTime}ms`);
console.log(`Total: ${fibTime + primeTime}ms`);
