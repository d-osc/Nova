// Test yield* (delegation) with generators

function* innerGen(): number {
    yield 1;
    yield 2;
    return 100;
}

function* outerGen(): number {
    yield 0;
    yield* innerGen();  // Should yield 1, 2
    yield 3;
    return 999;
}

function main(): number {
    console.log("Testing yield* delegation...");
    
    let gen = outerGen();
    let sum: number = 0;
    let count: number = 0;
    
    for (let value of gen) {
        console.log("Got value");
        sum = sum + value;
        count = count + 1;
    }
    
    // Should get: 0, 1, 2, 3 = sum 6, count 4
    if (count != 4) {
        console.log("FAIL: expected 4 values");
        return 1;
    }
    console.log("PASS: 4 values");
    
    if (sum != 6) {
        console.log("FAIL: expected sum = 6");
        return 1;
    }
    console.log("PASS: sum = 6");
    
    console.log("Test passed!");
    return 0;
}
