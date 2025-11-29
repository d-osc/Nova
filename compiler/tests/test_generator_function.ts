// Test generator function* declaration (ES2015)

function* simpleGenerator(): number {
    yield 10;
    yield 20;
    return 42;
}

function* rangeGenerator(n: number): number {
    yield n;
    yield n * 2;
    return n * 3;
}

function main(): number {
    console.log("Testing generator function...");

    // Test simple generator
    let gen1 = simpleGenerator();
    let r1 = gen1.next(0);
    if (r1.value != 10) {
        console.log("FAIL: expected first yield = 10");
        return 1;
    }

    let r2 = gen1.next(0);
    if (r2.value != 20) {
        console.log("FAIL: expected second yield = 20");
        return 1;
    }

    let r3 = gen1.next(0);
    if (r3.done != 1 || r3.value != 42) {
        console.log("FAIL: expected return value = 42");
        return 1;
    }
    console.log("PASS: simpleGenerator");

    // Test range generator with parameter
    let gen2 = rangeGenerator(5);
    let r4 = gen2.next(0);
    if (r4.value != 5) {
        console.log("FAIL: expected first yield = 5");
        return 1;
    }

    let r5 = gen2.next(0);
    if (r5.value != 10) {
        console.log("FAIL: expected second yield = 10");
        return 1;
    }

    let r6 = gen2.next(0);
    if (r6.done != 1 || r6.value != 15) {
        console.log("FAIL: expected return value = 15");
        return 1;
    }
    console.log("PASS: rangeGenerator");

    console.log("Test passed!");
    return 0;
}
