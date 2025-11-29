// Test async generator .return() method

async function* asyncCounter(): number {
    yield 10;
    yield 20;
    yield 30;
    return 99;
}

function main(): number {
    console.log("Testing async generator .return()...");

    let gen = asyncCounter();

    // Get first value
    let r1 = gen.next(0);
    if (r1.value != 10 || r1.done != 0) {
        console.log("FAIL: first yield");
        return 1;
    }
    console.log("PASS: first yield = 10");

    // Call .return() to terminate early
    let r2 = gen.return(42);
    if (r2.done != 1) {
        console.log("FAIL: return should set done=true");
        return 1;
    }
    if (r2.value != 42) {
        console.log("FAIL: return should set value=42");
        return 1;
    }
    console.log("PASS: return(42) works");

    // Subsequent .next() should return done=true
    let r3 = gen.next(0);
    if (r3.done != 1) {
        console.log("FAIL: after return, next should be done");
        return 1;
    }
    console.log("PASS: after return, generator is done");

    console.log("Test passed!");
    return 0;
}
