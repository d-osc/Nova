// Fibonacci(35) baseline benchmark
console.log("=== Fibonacci(35) Benchmark ===");

// Recursive implementation (baseline)
function fib_recursive(n) {
    if (n <= 1) return n;
    return fib_recursive(n - 1) + fib_recursive(n - 2);
}

console.log("\nTesting Fibonacci(35) - Recursive:");
let result = fib_recursive(35);
console.log("Result:", result);

// Iterative implementation
function fib_iterative(n) {
    if (n <= 1) return n;
    let a = 0;
    let b = 1;
    for (let i = 2; i <= n; i++) {
        let temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}

console.log("\nTesting Fibonacci(35) - Iterative:");
let result2 = fib_iterative(35);
console.log("Result:", result2);

console.log("\nBenchmark complete!");
