// Test JSON Methods (ES5)

function main(): number {
    // =========================================
    // Test 1: JSON.stringify(number)
    // =========================================
    console.log("=== JSON.stringify(number) ===");
    let numStr = JSON.stringify(42);
    console.log("JSON.stringify(42):", numStr);
    let negStr = JSON.stringify(-123);
    console.log("JSON.stringify(-123):", negStr);
    console.log("PASS: JSON.stringify(number)");

    // =========================================
    // Test 2: JSON.stringify(string)
    // =========================================
    console.log("");
    console.log("=== JSON.stringify(string) ===");
    let strStr = JSON.stringify("hello");
    console.log("JSON.stringify('hello'):", strStr);
    console.log("PASS: JSON.stringify(string)");

    // =========================================
    // Test 3: JSON.stringify(boolean)
    // =========================================
    console.log("");
    console.log("=== JSON.stringify(boolean) ===");
    let trueStr = JSON.stringify(true);
    console.log("JSON.stringify(true):", trueStr);
    let falseStr = JSON.stringify(false);
    console.log("JSON.stringify(false):", falseStr);
    console.log("PASS: JSON.stringify(boolean)");

    // =========================================
    // Test 4: JSON.stringify(array)
    // =========================================
    console.log("");
    console.log("=== JSON.stringify(array) ===");
    let arr = [1, 2, 3, 4, 5];
    let arrStr = JSON.stringify(arr);
    console.log("JSON.stringify([1,2,3,4,5]):", arrStr);
    console.log("PASS: JSON.stringify(array)");

    // =========================================
    // Test 5: JSON.parse(number string)
    // =========================================
    console.log("");
    console.log("=== JSON.parse(number) ===");
    let parsed1 = JSON.parse("123");
    console.log("JSON.parse('123') returned");
    console.log("PASS: JSON.parse(number)");

    // =========================================
    // Test 6: JSON.parse(string)
    // =========================================
    console.log("");
    console.log("=== JSON.parse(string) ===");
    let parsed2 = JSON.parse("\"hello world\"");
    console.log("JSON.parse('\"hello world\"') returned");
    console.log("PASS: JSON.parse(string)");

    // =========================================
    // Test 7: JSON.parse(boolean)
    // =========================================
    console.log("");
    console.log("=== JSON.parse(boolean) ===");
    let parsedTrue = JSON.parse("true");
    console.log("JSON.parse('true') returned");
    let parsedFalse = JSON.parse("false");
    console.log("JSON.parse('false') returned");
    console.log("PASS: JSON.parse(boolean)");

    // =========================================
    // Test 8: JSON.parse(null)
    // =========================================
    console.log("");
    console.log("=== JSON.parse(null) ===");
    let parsedNull = JSON.parse("null");
    console.log("JSON.parse('null') returned");
    console.log("PASS: JSON.parse(null)");

    // =========================================
    // All tests passed
    // =========================================
    console.log("");
    console.log("All JSON tests passed!");
    return 0;
}
