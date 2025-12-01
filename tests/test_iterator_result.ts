// Test IteratorResult value and done properties

function* countUp(): number {
    yield 10;
    yield 20;
    return 99;
}

function main(): number {
    console.log("Testing IteratorResult properties...");

    let gen = countUp();
    console.log("Generator created");

    // Get first result
    let result = gen.next(0);
    console.log("Got result from next()");

    // Access value property
    let val = result.value;
    console.log("Accessed result.value");

    // Access done property
    let isDone = result.done;
    console.log("Accessed result.done");

    console.log("Test passed!");
    return 0;
}
