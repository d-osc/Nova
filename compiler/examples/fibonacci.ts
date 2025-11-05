// Fibonacci benchmark
function fibonacci(n: number): number {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

const start = Date.now();
const result = fibonacci(40);
const end = Date.now();

console.log(`Fibonacci(40) = ${result}`);
console.log(`Time: ${end - start}ms`);
