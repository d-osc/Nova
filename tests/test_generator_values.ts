// Test generator values and done status

function* simpleGen(): number {
    yield 42;
    return 100;
}

function main(): number {
    console.log("Testing generator values...");

    let gen = simpleGen();

    // First call - should yield 42 and suspend (done=0)
    let result1 = gen.next(0);
    let isDone1 = result1.done;
    let val1 = result1.value;

    if (isDone1 != 0) {
        console.log("FAIL: expected done=0 after first yield");
        return 1;
    }
    console.log("PASS: done=0 after first yield");

    if (val1 != 42) {
        console.log("FAIL: expected value=42");
        return 1;
    }
    console.log("PASS: value=42");

    // Second call - should return 100 and complete (done=1)
    let result2 = gen.next(0);
    let isDone2 = result2.done;
    let val2 = result2.value;

    if (isDone2 != 1) {
        console.log("FAIL: expected done=1 after return");
        return 1;
    }
    console.log("PASS: done=1 after return");

    if (val2 != 100) {
        console.log("FAIL: expected value=100");
        return 1;
    }
    console.log("PASS: value=100");

    console.log("Test passed!");
    return 0;
}
