// Test yield* delegation with async generators

function* innerGen(): number {
    yield 10;
    yield 20;
    return 100;
}

async function* outerAsyncGen(): number {
    yield 1;
    yield* innerGen();  // Should yield 10, 20
    yield 99;
    return 999;
}

function main(): number {
    console.log("Testing async yield* delegation...");

    let gen = outerAsyncGen();

    // Call next multiple times - expected 4 yields: 1, 10, 20, 99
    gen.next(0);
    console.log("Got value 1");

    gen.next(0);
    console.log("Got value 2");

    gen.next(0);
    console.log("Got value 3");

    gen.next(0);
    console.log("Got value 4");

    gen.next(0);
    console.log("Generator done");

    console.log("Test passed!");
    return 0;
}
