// Comprehensive async generator test

async function* asyncCounter(): number {
    yield 10;
    yield 20;
    yield 30;
    return 100;
}

function* syncGen(): number {
    yield 1;
    yield 2;
}

async function* asyncWithYieldStar(): number {
    yield 0;
    yield* syncGen();  // yield* delegation
    yield 99;
    return 999;
}

function main(): number {
    console.log("Testing async generator comprehensive...");

    // Test 1: Basic async generator with .done/.value
    let gen1 = asyncCounter();
    let r1 = gen1.next(0);

    if (r1.done != 0) {
        console.log("FAIL: Test 1 - expected done=0");
        return 1;
    }
    if (r1.value != 10) {
        console.log("FAIL: Test 1 - expected value=10");
        return 1;
    }
    console.log("PASS: Test 1 - first yield");

    // Test 2: Continue iteration
    let r2 = gen1.next(0);
    if (r2.value != 20) {
        console.log("FAIL: Test 2 - expected value=20");
        return 1;
    }
    console.log("PASS: Test 2 - second yield");

    // Test 3: Async yield* delegation
    let gen2 = asyncWithYieldStar();
    let count = 0;
    let sum = 0;

    let r = gen2.next(0);
    while (r.done == 0) {
        sum = sum + r.value;
        count = count + 1;
        r = gen2.next(0);
    }

    if (count != 4) {
        console.log("FAIL: Test 3 - expected 4 values");
        return 1;
    }
    console.log("PASS: Test 3 - yield* produced 4 values");

    // sum should be 0 + 1 + 2 + 99 = 102
    if (sum != 102) {
        console.log("FAIL: Test 3 - expected sum=102");
        return 1;
    }
    console.log("PASS: Test 3 - yield* sum correct");

    console.log("All tests passed!");
    return 0;
}
