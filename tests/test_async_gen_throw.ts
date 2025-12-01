// Test async generator .throw() method

async function* asyncCounter(): number {
    yield 10;
    yield 20;
    yield 30;
    return 99;
}

function main(): number {
    console.log("Testing async generator .throw()...");

    let gen = asyncCounter();

    // Get first value
    let r1 = gen.next(0);
    if (r1.value != 10 || r1.done != 0) {
        console.log("FAIL: first yield");
        return 1;
    }
    console.log("PASS: first yield = 10");

    // Call .throw() to throw an error into the generator
    let r2 = gen.throw(999);
    // After throw, generator should be done
    if (r2.done != 1) {
        console.log("FAIL: throw should set done=true");
        return 1;
    }
    console.log("PASS: throw() terminates generator");

    // Subsequent .next() should return done=true
    let r3 = gen.next(0);
    if (r3.done != 1) {
        console.log("FAIL: after throw, next should be done");
        return 1;
    }
    console.log("PASS: after throw, generator is done");

    console.log("Test passed!");
    return 0;
}
