// Test await inside async generator

async function* asyncGenWithAwait(): number {
    yield 10;
    // Simulate await (in real code would await a promise)
    let x: number = 20;
    yield x;
    yield 30;
    return 99;
}

function main(): number {
    console.log("Testing async generator with values...");
    
    let gen = asyncGenWithAwait();
    let sum: number = 0;
    let count: number = 0;
    
    for await (let value of gen) {
        console.log("Got value");
        sum = sum + value;
        count = count + 1;
    }
    
    if (count != 3) {
        console.log("FAIL: expected 3 values");
        return 1;
    }
    console.log("PASS: 3 values");
    
    if (sum != 60) {
        console.log("FAIL: expected sum = 60");
        return 1;
    }
    console.log("PASS: sum = 60");
    
    console.log("Test passed!");
    return 0;
}
