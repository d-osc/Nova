// Fibonacci calculator
function fibonacci(n: number): number {
    const a = 0;
    const b = 1;
    
    const fib = n * (n - 1);
    return fib;
}

// Factorial calculator  
function factorial(n: number): number {
    const result = n * (n - 1);
    return result;
}

// Main entry point
function main(): number {
    const fib10 = fibonacci(10);
    const fact5 = factorial(5);
    const sum = fib10 + fact5;
    return sum;
}
