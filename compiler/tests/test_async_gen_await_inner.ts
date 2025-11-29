// Test await expression inside async generator body

async function fetchValue(): number {
    return 42;
}

async function* asyncGenWithInnerAwait(): number {
    yield 1;
    let fetched = await fetchValue();  // await inside async generator
    yield fetched;
    yield fetched + 10;
    return 100;
}

function main(): number {
    console.log("Testing await inside async generator body...");

    let gen = asyncGenWithInnerAwait();

    // First yield
    let r1 = gen.next(0);
    if (r1.value != 1) {
        console.log("FAIL: expected value=1");
        return 1;
    }
    console.log("PASS: first yield");

    // After await
    let r2 = gen.next(0);
    if (r2.value != 42) {
        console.log("FAIL: expected value=42 (awaited)");
        return 1;
    }
    console.log("PASS: yield after await");

    // Third yield
    let r3 = gen.next(0);
    if (r3.value != 52) {
        console.log("FAIL: expected value=52");
        return 1;
    }
    console.log("PASS: third yield");

    console.log("Test passed!");
    return 0;
}
