// Test case: Deep function call chain
function f1(x: number): number {
    return x + 1;
}

function f2(x: number): number {
    return f1(x) * 2;
}

function f3(x: number): number {
    return f2(f1(x)) + 3;
}

function f4(x: number): number {
    return f3(f2(f1(x)));
}
