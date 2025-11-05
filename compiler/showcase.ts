// Nova Compiler - Showcase Examples

// Example 1: Basic arithmetic
function basicMath(x: number, y: number): number {
    const sum = x + y;
    const diff = x - y;
    const prod = sum * diff;
    return prod;
}

// Example 2: Expression composition
function compose(a: number, b: number, c: number): number {
    const temp1 = a + b;
    const temp2 = b + c;
    const result = temp1 * temp2;
    return result;
}

// Example 3: Multiple function calls
function helper1(n: number): number {
    return n * 2;
}

function helper2(n: number): number {
    return n + 10;
}

function combined(): number {
    const x = helper1(5);
    const y = helper2(3);
    const z = x + y;
    return z;
}

// Example 4: Deep nesting
function add(a: number, b: number): number {
    return a + b;
}

function multiply(a: number, b: number): number {
    return a * b;
}

function complex(): number {
    return multiply(add(1, 2), add(3, 4));
}
