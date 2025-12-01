function multiply(x: number, y: number): number {
    return x * y;
}

function calculate(): number {
    const a = 10;
    const b = 5;
    const result1 = multiply(a, b);
    const result2 = multiply(result1, 2);
    return result2;
}
