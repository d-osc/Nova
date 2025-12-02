// Nova-compatible benchmark
function add(a: number, b: number): number {
    return a + b;
}

function multiply(a: number, b: number): number {
    return a * b;
}

function compute(): number {
    let result = 0;
    result = add(1, 2);
    result = multiply(result, 3);
    result = add(result, 4);
    result = multiply(result, 5);
    return result;
}

const answer = compute();
console.log(answer);
