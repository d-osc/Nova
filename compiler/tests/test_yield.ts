// Test yield expression (ES2015)

function* simpleGenerator(): number {
    let x: number = 10;
    yield x;
    return x * 2;
}

function main(): number {
    let gen = simpleGenerator();

    // First call - yield 10
    let r1 = gen.next(0);
    if (r1.value != 10) {
        console.log("FAIL: expected first value = 10");
        return 1;
    }
    if (r1.done != 0) {
        console.log("FAIL: expected done=0 after yield");
        return 1;
    }

    // Second call - return 20
    let r2 = gen.next(0);
    if (r2.value != 20) {
        console.log("FAIL: expected return value = 20");
        return 1;
    }
    if (r2.done != 1) {
        console.log("FAIL: expected done=1 after return");
        return 1;
    }

    console.log("Test passed!");
    return 0;
}
