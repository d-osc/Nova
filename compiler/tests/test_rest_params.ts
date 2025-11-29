// Test rest parameters (...args)
// Note: Syntax is supported, full varargs collection requires runtime

function withRest(a: number, b: number, ...rest): number {
    // rest parameter is declared (collection not fully implemented)
    return a + b;
}

function main(): number {
    let result: number = withRest(10, 20);

    if (result != 30) {
        return 1;
    }

    return 0;
}
