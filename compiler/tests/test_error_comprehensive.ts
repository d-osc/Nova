// Comprehensive error test
function main(): number {
    let passed = 0;
    let failed = 0;

    // Test 1: Create Error with message
    let e1 = new Error("Test error message");
    console.log("Test 1: Error created");
    passed = passed + 1;

    // Test 2: Create TypeError
    let e2 = new TypeError("Type mismatch");
    console.log("Test 2: TypeError created");
    passed = passed + 1;

    // Test 3: Create RangeError
    let e3 = new RangeError("Value out of range");
    console.log("Test 3: RangeError created");
    passed = passed + 1;

    // Test 4: Create ReferenceError
    let e4 = new ReferenceError("Undefined variable");
    console.log("Test 4: ReferenceError created");
    passed = passed + 1;

    // Test 5: Create SyntaxError
    let e5 = new SyntaxError("Invalid syntax");
    console.log("Test 5: SyntaxError created");
    passed = passed + 1;

    // Test 6: Create URIError
    let e6 = new URIError("Invalid URI");
    console.log("Test 6: URIError created");
    passed = passed + 1;

    // Test 7: Create EvalError
    let e7 = new EvalError("Eval failed");
    console.log("Test 7: EvalError created");
    passed = passed + 1;

    // Test 8: Create InternalError
    let e8 = new InternalError("Internal problem");
    console.log("Test 8: InternalError created");
    passed = passed + 1;

    // Test 9: Create AggregateError
    let e9 = new AggregateError("Multiple errors");
    console.log("Test 9: AggregateError created");
    passed = passed + 1;

    // Test 10: Create Error without message
    let e10 = new Error();
    console.log("Test 10: Error without message created");
    passed = passed + 1;

    console.log("=== Results ===");
    console.log(passed);
    console.log(failed);

    return failed;
}
