// Test AggregateError with errors array
function main(): number {
    console.log("Testing AggregateError...");

    // Create some errors
    let e1 = new Error("First error");
    let e2 = new TypeError("Second error");
    let e3 = new RangeError("Third error");

    // Test 1: AggregateError with errors array and message
    let agg1 = new AggregateError([e1, e2, e3], "Multiple errors occurred");
    console.log("Test 1 PASS: AggregateError with array and message");

    // Test 2: AggregateError with just errors array
    let agg2 = new AggregateError([e1, e2]);
    console.log("Test 2 PASS: AggregateError with array only");

    // Test 3: AggregateError with empty array
    let agg3 = new AggregateError([], "No errors");
    console.log("Test 3 PASS: AggregateError with empty array");

    // Test 4: AggregateError with just message (backwards compat)
    let agg4 = new AggregateError("Just a message");
    console.log("Test 4 PASS: AggregateError with message only");

    console.log("All AggregateError tests passed!");
    return 0;
}
