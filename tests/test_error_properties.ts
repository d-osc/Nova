// Test Error properties and methods (ES1)

function main(): number {
    // =========================================
    // Test 1: Create Error with message
    // =========================================
    let err1 = new Error("Something went wrong");
    console.log("Created Error with message");
    console.log("PASS: Error constructor");

    // =========================================
    // Test 2: Error.name property
    // =========================================
    let name1 = err1.name;
    console.log("err1.name:", name1);  // "Error"
    console.log("PASS: Error.name");

    // =========================================
    // Test 3: Error.message property
    // =========================================
    let msg1 = err1.message;
    console.log("err1.message:", msg1);  // "Something went wrong"
    console.log("PASS: Error.message");

    // =========================================
    // Test 4: Error.stack property
    // =========================================
    let stack1 = err1.stack;
    console.log("err1.stack:", stack1);
    console.log("PASS: Error.stack");

    // =========================================
    // Test 5: Error.toString() method
    // =========================================
    let str1 = err1.toString();
    console.log("err1.toString():", str1);  // "Error: Something went wrong"
    console.log("PASS: Error.toString()");

    // =========================================
    // Test 6: Error with empty message
    // =========================================
    let err2 = new Error("");
    console.log("Created Error with empty message");
    let str2 = err2.toString();
    console.log("err2.toString():", str2);  // "Error"
    console.log("PASS: Error with empty message");

    // =========================================
    // Test 7: TypeError
    // =========================================
    let typeErr = new TypeError("Type mismatch");
    console.log("Created TypeError");
    let typeName = typeErr.name;
    let typeMsg = typeErr.message;
    let typeStr = typeErr.toString();
    console.log("typeErr.name:", typeName);  // "TypeError"
    console.log("typeErr.message:", typeMsg);  // "Type mismatch"
    console.log("typeErr.toString():", typeStr);  // "TypeError: Type mismatch"
    console.log("PASS: TypeError");

    // =========================================
    // Test 8: RangeError
    // =========================================
    let rangeErr = new RangeError("Value out of range");
    console.log("Created RangeError");
    let rangeName = rangeErr.name;
    let rangeMsg = rangeErr.message;
    let rangeStr = rangeErr.toString();
    console.log("rangeErr.name:", rangeName);  // "RangeError"
    console.log("rangeErr.message:", rangeMsg);  // "Value out of range"
    console.log("rangeErr.toString():", rangeStr);  // "RangeError: Value out of range"
    console.log("PASS: RangeError");

    // =========================================
    // Test 9: ReferenceError
    // =========================================
    let refErr = new ReferenceError("Variable not defined");
    console.log("Created ReferenceError");
    let refName = refErr.name;
    let refMsg = refErr.message;
    console.log("refErr.name:", refName);  // "ReferenceError"
    console.log("refErr.message:", refMsg);  // "Variable not defined"
    console.log("PASS: ReferenceError");

    // =========================================
    // Test 10: SyntaxError
    // =========================================
    let syntaxErr = new SyntaxError("Unexpected token");
    console.log("Created SyntaxError");
    let syntaxName = syntaxErr.name;
    let syntaxMsg = syntaxErr.message;
    console.log("syntaxErr.name:", syntaxName);  // "SyntaxError"
    console.log("syntaxErr.message:", syntaxMsg);  // "Unexpected token"
    console.log("PASS: SyntaxError");

    // =========================================
    // Test 11: URIError
    // =========================================
    let uriErr = new URIError("Invalid URI");
    console.log("Created URIError");
    let uriName = uriErr.name;
    let uriMsg = uriErr.message;
    console.log("uriErr.name:", uriName);  // "URIError"
    console.log("uriErr.message:", uriMsg);  // "Invalid URI"
    console.log("PASS: URIError");

    // =========================================
    // Test 12: EvalError
    // =========================================
    let evalErr = new EvalError("Eval failed");
    console.log("Created EvalError");
    let evalName = evalErr.name;
    let evalMsg = evalErr.message;
    console.log("evalErr.name:", evalName);  // "EvalError"
    console.log("evalErr.message:", evalMsg);  // "Eval failed"
    console.log("PASS: EvalError");

    // =========================================
    // All tests passed
    // =========================================
    console.log("");
    console.log("All Error property and method tests passed!");
    return 0;
}
