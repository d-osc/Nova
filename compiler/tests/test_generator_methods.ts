// Test generator methods (ES2015)
// next(), return(), throw()

function* countGen(): number {
    yield 1;
    yield 2;
    return 99;
}

function main(): number {
    console.log("Testing generator methods...");

    // Create generator
    let gen = countGen();
    console.log("Generator created");

    // Test next()
    let r1 = gen.next(0);
    console.log("next() called");

    // Test return()
    let r2 = gen.return(42);
    console.log("return() called");

    // Test throw()
    let r3 = gen.throw(100);
    console.log("throw() called");

    console.log("Test passed!");
    return 0;
}
