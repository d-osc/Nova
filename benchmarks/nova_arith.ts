// Benchmark 1: Basic Arithmetic Operations
function add(a: number, b: number): number {
    return a + b;
}

function sub(a: number, b: number): number {
    return a - b;
}

function mul(a: number, b: number): number {
    return a * b;
}

function div(a: number, b: number): number {
    return a / b;
}

function compute(): number {
    const a = add(100, 200);
    const b = sub(a, 50);
    const c = mul(b, 3);
    const d = div(c, 2);
    return d;
}

const result = compute();
console.log(result);
