// Test basic Error constructor
function main(): number {
    console.log("Testing Error types...");

    // Create Error objects
    let e1 = new Error("Something went wrong");
    console.log("Created Error");

    let e2 = new TypeError("Not a function");
    console.log("Created TypeError");

    let e3 = new RangeError("Index out of bounds");
    console.log("Created RangeError");

    let e4 = new ReferenceError("Variable not defined");
    console.log("Created ReferenceError");

    let e5 = new SyntaxError("Unexpected token");
    console.log("Created SyntaxError");

    let e6 = new URIError("Malformed URI");
    console.log("Created URIError");

    console.log("All error types created successfully!");
    return 0;
}
