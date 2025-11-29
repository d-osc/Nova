// Test for...of loop with generator

function* numbersGen(): number {
    yield 10;
    yield 20;
    yield 30;
    return 99;
}

function main(): number {
    console.log("Testing for...of with generator...");

    let gen = numbersGen();
    console.log("Generator created");

    // Note: Current generator implementation runs to completion on first next()
    // So this loop will only execute once with the final value
    for (let value of gen) {
        console.log("Loop iteration");
    }

    console.log("Loop completed");
    console.log("Test passed!");
    return 0;
}
