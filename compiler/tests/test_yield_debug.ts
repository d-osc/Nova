// Debug test for yield

function* simpleYield(): number {
    console.log("Before yield");
    yield 10;
    console.log("After yield");
    return 99;
}

function main(): number {
    console.log("Creating generator...");
    let gen = simpleYield();

    console.log("Calling next()...");
    let result = gen.next(0);

    console.log("Checking done...");
    let done = result.done;

    console.log("Checking value...");
    let value = result.value;

    if (done == 0) {
        console.log("Done is 0 (correct)");
    } else {
        console.log("Done is not 0");
    }

    if (value == 10) {
        console.log("Value is 10 (correct)");
    } else {
        console.log("Value is not 10");
    }

    console.log("Test complete");
    return 0;
}
