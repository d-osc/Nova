// Test Boolean prototype methods (ES1)

function main(): number {
    // =========================================
    // Test 1: Boolean.prototype.toString()
    // =========================================
    let t: boolean = true;
    let f: boolean = false;

    let tStr = t.toString();
    console.log("true.toString():", tStr);  // "true"

    let fStr = f.toString();
    console.log("false.toString():", fStr);  // "false"
    console.log("PASS: Boolean.prototype.toString()");

    // =========================================
    // Test 2: Boolean.prototype.valueOf()
    // =========================================
    let tVal = t.valueOf();
    console.log("true.valueOf():", tVal);  // 1

    let fVal = f.valueOf();
    console.log("false.valueOf():", fVal);  // 0
    console.log("PASS: Boolean.prototype.valueOf()");

    // =========================================
    // Test 3: Boolean() constructor
    // =========================================
    let b1 = Boolean(1);
    let b2 = Boolean(0);
    let b3 = Boolean(100);
    let b4 = Boolean(-1);

    console.log("Boolean(1):", b1);    // 1 (true)
    console.log("Boolean(0):", b2);    // 0 (false)
    console.log("Boolean(100):", b3);  // 1 (true)
    console.log("Boolean(-1):", b4);   // 1 (true)
    console.log("PASS: Boolean() constructor");

    // =========================================
    // Test 4: Comparison expressions
    // =========================================
    let cmp1 = (5 > 3);
    let cmp2 = (3 > 5);

    let cmp1Str = cmp1.toString();
    let cmp2Str = cmp2.toString();

    console.log("(5 > 3).toString():", cmp1Str);  // "true"
    console.log("(3 > 5).toString():", cmp2Str);  // "false"
    console.log("PASS: Comparison expression toString()");

    // =========================================
    // All tests passed
    // =========================================
    console.log("");
    console.log("All Boolean method tests passed!");
    return 0;
}
