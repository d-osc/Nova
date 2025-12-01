// Test eval() function (ES1 - AOT limited)

function main(): number {
    // =========================================
    // Test 1: eval with numeric literals
    // =========================================
    let num1 = eval("42");
    console.log("eval('42'):", num1);
    console.log("PASS: eval numeric literal");

    // =========================================
    // Test 2: eval with negative number
    // =========================================
    let num2 = eval("-123");
    console.log("eval('-123'):", num2);
    console.log("PASS: eval negative number");

    // =========================================
    // Test 3: eval with boolean true
    // =========================================
    let bool1 = eval("true");
    console.log("eval('true'):", bool1);
    console.log("PASS: eval boolean true");

    // =========================================
    // Test 4: eval with boolean false
    // =========================================
    let bool2 = eval("false");
    console.log("eval('false'):", bool2);
    console.log("PASS: eval boolean false");

    // =========================================
    // Test 5: eval with null
    // =========================================
    let nullVal = eval("null");
    console.log("eval('null'):", nullVal);
    console.log("PASS: eval null");

    // =========================================
    // Test 6: eval with undefined
    // =========================================
    let undefVal = eval("undefined");
    console.log("eval('undefined'):", undefVal);
    console.log("PASS: eval undefined");

    // =========================================
    // Test 7: eval with addition
    // =========================================
    let add = eval("10 + 5");
    console.log("eval('10 + 5'):", add);
    console.log("PASS: eval addition");

    // =========================================
    // Test 8: eval with subtraction
    // =========================================
    let sub = eval("20 - 8");
    console.log("eval('20 - 8'):", sub);
    console.log("PASS: eval subtraction");

    // =========================================
    // Test 9: eval with multiplication
    // =========================================
    let mul = eval("6 * 7");
    console.log("eval('6 * 7'):", mul);
    console.log("PASS: eval multiplication");

    // =========================================
    // Test 10: eval with division
    // =========================================
    let div = eval("100 / 4");
    console.log("eval('100 / 4'):", div);
    console.log("PASS: eval division");

    // =========================================
    // Test 11: eval with modulo
    // =========================================
    let mod = eval("17 % 5");
    console.log("eval('17 % 5'):", mod);
    console.log("PASS: eval modulo");

    // =========================================
    // Test 12: eval with no arguments
    // =========================================
    let noArg = eval();
    console.log("eval():", noArg);
    console.log("PASS: eval no arguments");

    // =========================================
    // Test 13: eval with whitespace
    // =========================================
    let ws = eval("  100  ");
    console.log("eval('  100  '):", ws);
    console.log("PASS: eval with whitespace");

    // =========================================
    // All tests passed
    // =========================================
    console.log("");
    console.log("All eval() tests passed!");
    console.log("Note: eval() in Nova AOT only supports constant expressions.");
    return 0;
}
