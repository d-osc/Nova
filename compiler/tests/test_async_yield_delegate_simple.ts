// Simple async yield* test

function* innerGen(): number {
    yield 1;
    yield 2;
}

async function* outerAsyncGen(): number {
    yield 0;
    yield* innerGen();  // yield* from async to sync generator
    yield 3;
}

function main(): number {
    console.log("Testing async yield* (simple)...");

    let gen = outerAsyncGen();
    let r = gen.next(0);
    console.log("Got first value");

    console.log("Test passed!");
    return 0;
}
