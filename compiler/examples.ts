// Nova Compiler - Example Collection
// All working examples demonstrating compiler features

// ============================================
// Example 1: Basic Arithmetic
// ============================================
function basicAdd(a: number, b: number): number {
    return a + b;
}

function basicSubtract(a: number, b: number): number {
    return a - b;
}

function basicMultiply(a: number, b: number): number {
    return a * b;
}

function basicDivide(a: number, b: number): number {
    return a / b;
}

// ============================================
// Example 2: Combined Operations
// ============================================
function allOperations(a: number, b: number, c: number): number {
    const sum = a + b;
    const diff = sum - c;
    const product = diff * 2;
    const result = product / 2;
    return result;
}

// ============================================
// Example 3: Function Composition
// ============================================
function double(x: number): number {
    return x * 2;
}

function triple(x: number): number {
    return x * 3;
}

function quadruple(x: number): number {
    return double(double(x));
}

function compose(x: number): number {
    return double(x) + triple(x);
}

// ============================================
// Example 4: Nested Calls
// ============================================
function inner(x: number): number {
    return x + 1;
}

function middle(x: number): number {
    return inner(x) * 2;
}

function outer(x: number): number {
    return middle(inner(x));
}

// ============================================
// Example 5: Multiple Parameters
// ============================================
function sum2(a: number, b: number): number {
    return a + b;
}

function sum3(a: number, b: number, c: number): number {
    return sum2(sum2(a, b), c);
}

function sum4(a: number, b: number, c: number, d: number): number {
    return sum2(sum2(a, b), sum2(c, d));
}

// ============================================
// Example 6: Chained Calculations
// ============================================
function increment(x: number): number {
    return x + 1;
}

function decrement(x: number): number {
    return x - 1;
}

function chain(start: number): number {
    const step1 = increment(start);
    const step2 = double(step1);
    const step3 = increment(step2);
    const step4 = triple(step3);
    return step4;
}

// ============================================
// Example 7: Mathematical Expressions
// ============================================
function expression1(): number {
    return 1 + 2 * 3;
}

function expression2(a: number, b: number): number {
    return a * b + a / b;
}

function expression3(x: number): number {
    const a = x + 1;
    const b = x - 1;
    const c = a * b;
    return c;
}

// ============================================
// Example 8: Complex Nesting
// ============================================
function add(a: number, b: number): number {
    return a + b;
}

function multiply(a: number, b: number): number {
    return a * b;
}

function complexNested(): number {
    return multiply(
        add(1, 2),
        add(3, multiply(4, 5))
    );
}

// ============================================
// Example 9: Recursive-style (Iterative)
// ============================================
function factorial5(): number {
    const n1 = 1;
    const n2 = n1 * 2;
    const n3 = n2 * 3;
    const n4 = n3 * 4;
    const n5 = n4 * 5;
    return n5;
}

function fibonacci7(): number {
    const f0 = 0;
    const f1 = 1;
    const f2 = f0 + f1;
    const f3 = f1 + f2;
    const f4 = f2 + f3;
    const f5 = f3 + f4;
    const f6 = f4 + f5;
    const f7 = f5 + f6;
    return f7;
}

// ============================================
// Example 10: Main Entry Point
// ============================================
function main(): number {
    const basic = basicAdd(10, 20);
    const composed = compose(5);
    const nested = outer(3);
    const chained = chain(1);
    const complex = complexNested();
    
    const total = sum4(basic, composed, nested, chained);
    return total + complex;
}
