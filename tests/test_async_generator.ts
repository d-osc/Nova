// Test async generator function (async function*)
// ES2018 feature

async function* asyncCounter(): number {
    yield 10;
    yield 20;
    yield 30;
    return 99;
}

function main(): number {
    console.log("Testing async generator...");

    // Create async generator
    let gen = asyncCounter();
    console.log("Async generator created");

    // Call next() - returns Promise<IteratorResult>
    // For now, we just test that next() can be called
    let result = gen.next(0);
    console.log("First next() called");

    // Second call
    let result2 = gen.next(0);
    console.log("Second next() called");

    // Third call
    let result3 = gen.next(0);
    console.log("Third next() called");

    console.log("Test passed!");
    return 0;
}
