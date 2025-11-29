// Simple async generator test without yield*

async function* simpleAsyncGen(): number {
    yield 1;
    yield 2;
    yield 3;
}

function main(): number {
    console.log("Testing simple async generator...");

    let gen = simpleAsyncGen();
    let r = gen.next(0);

    console.log("Test passed!");
    return 0;
}
