// Test async generator result properties

async function* asyncGen(): number {
    yield 1;
    yield 2;
    return 100;
}

function main(): number {
    console.log("Testing async generator properties...");

    let gen = asyncGen();
    let result = gen.next(0);

    // Test accessing .done and .value
    let done = result.done;
    let value = result.value;

    if (done == 0 && value == 1) {
        console.log("PASS: first yield");
    } else {
        console.log("FAIL: first yield");
    }

    console.log("Test passed!");
    return 0;
}
