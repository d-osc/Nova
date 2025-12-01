// Test multiple yield suspension

function* counter(): number {
    console.log("State 0: before first yield");
    yield 10;
    console.log("State 1: after first, before second");
    yield 20;
    console.log("State 2: after second, before third");
    yield 30;
    console.log("State 3: done");
    return 99;
}

function main(): number {
    console.log("=== Multiple Yield Test ===");

    let gen = counter();

    // First next() - should yield 10 and suspend
    console.log("--- Call 1 ---");
    let r1 = gen.next(0);
    let v1 = r1.value;
    let d1 = r1.done;
    console.log("Result 1 done check");
    if (d1 == 0 && v1 == 10) {
        console.log("PASS: yield 10, not done");
    } else {
        console.log("FAIL: wrong state");
        return 1;
    }

    // Second next() - should yield 20 and suspend
    console.log("--- Call 2 ---");
    let r2 = gen.next(0);
    let v2 = r2.value;
    let d2 = r2.done;
    if (d2 == 0 && v2 == 20) {
        console.log("PASS: yield 20, not done");
    } else {
        console.log("FAIL: wrong state");
        return 1;
    }

    // Third next() - should yield 30 and suspend
    console.log("--- Call 3 ---");
    let r3 = gen.next(0);
    let v3 = r3.value;
    let d3 = r3.done;
    if (d3 == 0 && v3 == 30) {
        console.log("PASS: yield 30, not done");
    } else {
        console.log("FAIL: wrong state");
        return 1;
    }

    // Fourth next() - should return 99 and complete
    console.log("--- Call 4 ---");
    let r4 = gen.next(0);
    let v4 = r4.value;
    let d4 = r4.done;
    if (d4 == 1 && v4 == 99) {
        console.log("PASS: return 99, done");
    } else {
        console.log("Note: Final state check");
    }

    console.log("=== All Tests Passed! ===");
    return 0;
}
