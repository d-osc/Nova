// Benchmark 2: Nested Function Calls
function inner(x: number): number {
    return x + 1;
}

function middle(x: number): number {
    return inner(x) * 2;
}

function outer(x: number): number {
    return middle(inner(x));
}

function deep1(x: number): number {
    return outer(middle(inner(x)));
}

function deep2(x: number): number {
    return deep1(outer(middle(x)));
}

const result = deep2(5);
console.log(result);
