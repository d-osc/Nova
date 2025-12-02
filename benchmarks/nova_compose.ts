// Benchmark 3: Function Composition
function dbl(x: number): number {
    return x * 2;
}

function trpl(x: number): number {
    return x * 3;
}

function quad(x: number): number {
    const a = dbl(x);
    const b = dbl(a);
    return b;
}

function compose1(x: number): number {
    const a = quad(x);
    const b = trpl(a);
    const c = dbl(b);
    return c;
}

const result = compose1(5);
console.log(result);
