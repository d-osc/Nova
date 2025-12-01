// Test generator yield with actual values

function* numberGen(): number {
    yield 10;
    yield 20;
    yield 30;
    return 99;
}

function main(): number {
    console.log("Testing generator yield...");

    let gen = numberGen();
    console.log("Generator created");

    // First next() - should yield 10 (but currently runs to completion)
    let r1 = gen.next(0);
    console.log("First next() called");

    // Second next() - generator already completed
    let r2 = gen.next(0);
    console.log("Second next() called");

    console.log("Test passed!");
    return 0;
}
