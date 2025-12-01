// Test for await...of with async generator (ES2018)

async function* asyncCounter(): number {
    yield 10;
    yield 20;
    yield 30;
    return 99;
}

function main(): number {
    console.log("Testing for await...of with async generator...");

    let gen = asyncCounter();
    let count: number = 0;
    let sum: number = 0;

    // Use for await...of to iterate over async generator
    for await (let value of gen) {
        console.log("Got value");
        count = count + 1;
        sum = sum + value;
    }

    console.log("Loop finished");

    // Should have iterated 3 times (10, 20, 30)
    if (count != 3) {
        console.log("FAIL: expected 3 iterations");
        return 1;
    }
    console.log("PASS: 3 iterations");

    // Sum should be 60 (10 + 20 + 30)
    if (sum != 60) {
        console.log("FAIL: expected sum = 60");
        return 1;
    }
    console.log("PASS: sum = 60");

    console.log("Test passed!");
    return 0;
}
