// Test case: Expression complexity
function expr1(a: number, b: number): number {
    return a + b * 2;
}

function expr2(a: number, b: number, c: number): number {
    return a * b + c / 2;
}

function expr3(x: number): number {
    return x * 2 + x / 2 - x;
}

function complex(a: number, b: number, c: number): number {
    return expr1(a, b) + expr2(b, c, a) - expr3(c);
}
