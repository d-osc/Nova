function add(a: number, b: number): number {
    return a + b;
}

function multiply(x: number, y: number): number {
    return x * y;
}

function compute(): number {
    return multiply(add(2, 3), add(4, 5));
}
