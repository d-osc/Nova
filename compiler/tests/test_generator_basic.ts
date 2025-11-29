// Test basic generator function (ES2015)

function* simpleGenerator(): number {
    yield 1;
    yield 2;
    yield 3;
    return 10;
}

function main(): number {
    console.log("Testing basic generator...");

    // Create generator object
    let gen = simpleGenerator();
    console.log("Generator created");

    // Call next() to get values
    let result = gen.next(0);
    console.log("First next() called");

    console.log("Test passed!");
    return 0;
}
