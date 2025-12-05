// Test Fibonacci with smaller numbers first
console.log("Testing Fibonacci");

function fib(n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

console.log("fib(5):");
console.log(fib(5));

console.log("fib(10):");
console.log(fib(10));

console.log("fib(15):");
console.log(fib(15));

console.log("fib(20):");
console.log(fib(20));

console.log("Done!");
