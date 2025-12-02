// Benchmark 5: Complex Expression Evaluation
function calc1(a: number, b: number, c: number): number {
    const ab = a * b;
    const bc = b * c;
    const ca = c * a;
    const sum1 = ab + bc;
    return sum1 + ca;
}

function calc2(x: number, y: number): number {
    const xx = x * x;
    const yy = y * y;
    return xx + yy;
}

function combine(a: number, b: number, c: number): number {
    const r1 = calc1(a, b, c);
    const r2 = calc2(a, b);
    return r1 + r2;
}

const result = combine(10, 20, 30);
console.log(result);
