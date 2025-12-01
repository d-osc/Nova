// Comprehensive test for Boolean (ES1)

function main(): number {
    // =========================================
    // Test 1: Boolean literals
    // =========================================
    let t = true;
    let f = false;
    console.log("true literal:", t);
    console.log("false literal:", f);
    console.log("PASS: Boolean literals");

    // =========================================
    // Test 2: Boolean() constructor with numbers
    // =========================================
    let b0 = Boolean(0);
    let b1 = Boolean(1);
    let bNeg = Boolean(-100);
    let bPos = Boolean(42);

    console.log("Boolean(0):", b0);      // 0 (false)
    console.log("Boolean(1):", b1);      // 1 (true)
    console.log("Boolean(-100):", bNeg); // 1 (true)
    console.log("Boolean(42):", bPos);   // 1 (true)
    console.log("PASS: Boolean constructor");

    // =========================================
    // Test 3: Boolean.prototype.toString()
    // =========================================
    let trueStr = t.toString();
    let falseStr = f.toString();

    console.log("true.toString():", trueStr);   // "true"
    console.log("false.toString():", falseStr); // "false"
    console.log("PASS: Boolean.prototype.toString()");

    // =========================================
    // Test 4: Boolean.prototype.valueOf()
    // =========================================
    let trueVal = t.valueOf();
    let falseVal = f.valueOf();

    console.log("true.valueOf():", trueVal);   // 1
    console.log("false.valueOf():", falseVal); // 0
    console.log("PASS: Boolean.prototype.valueOf()");

    // =========================================
    // Test 5: Logical operators
    // =========================================
    let andTT = true && true;
    let andTF = true && false;
    let andFT = false && true;
    let andFF = false && false;

    console.log("true && true:", andTT);   // 1
    console.log("true && false:", andTF);  // 0
    console.log("false && true:", andFT);  // 0
    console.log("false && false:", andFF); // 0
    console.log("PASS: Logical AND");

    let orTT = true || true;
    let orTF = true || false;
    let orFT = false || true;
    let orFF = false || false;

    console.log("true || true:", orTT);   // 1
    console.log("true || false:", orTF);  // 1
    console.log("false || true:", orFT);  // 1
    console.log("false || false:", orFF); // 0
    console.log("PASS: Logical OR");

    let notT = !true;
    let notF = !false;

    console.log("!true:", notT);  // 0
    console.log("!false:", notF); // 1
    console.log("PASS: Logical NOT");

    // =========================================
    // Test 6: Comparison expressions to boolean
    // =========================================
    let eq = (5 == 5);
    let neq = (5 != 3);
    let lt = (3 < 5);
    let gt = (5 > 3);
    let lte = (3 <= 3);
    let gte = (5 >= 5);

    console.log("5 == 5:", eq);  // 1
    console.log("5 != 3:", neq); // 1
    console.log("3 < 5:", lt);   // 1
    console.log("5 > 3:", gt);   // 1
    console.log("3 <= 3:", lte); // 1
    console.log("5 >= 5:", gte); // 1
    console.log("PASS: Comparison operators");

    // =========================================
    // Test 7: Boolean in conditionals
    // =========================================
    let result = 0;
    if (true) {
        result = 1;
    }
    console.log("if (true) result:", result); // 1

    result = 0;
    if (false) {
        result = 1;
    }
    console.log("if (false) result:", result); // 0
    console.log("PASS: Boolean in conditionals");

    // =========================================
    // Test 8: Ternary with boolean
    // =========================================
    let tern1 = true ? 100 : 200;
    let tern2 = false ? 100 : 200;

    console.log("true ? 100 : 200:", tern1);  // 100
    console.log("false ? 100 : 200:", tern2); // 200
    console.log("PASS: Ternary operator");

    // =========================================
    // All tests passed
    // =========================================
    console.log("");
    console.log("All Boolean comprehensive tests passed!");
    return 0;
}
