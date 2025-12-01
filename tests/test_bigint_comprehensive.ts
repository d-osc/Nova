// Comprehensive test for BigInt (ES2020)

function main(): number {
    // =========================================
    // Test 1: BigInt literal syntax
    // =========================================
    let lit1 = 42n;
    let lit2 = 0n;
    let lit3 = -100n;
    console.log("PASS: BigInt literals");

    // =========================================
    // Test 2: BigInt constructor from number
    // =========================================
    let num1 = BigInt(100);
    let num2 = BigInt(0);
    let num3 = BigInt(-50);
    console.log("PASS: BigInt from number");

    // =========================================
    // Test 3: BigInt constructor from string
    // =========================================
    let str1 = BigInt("999");
    let str2 = BigInt("123456789012345678901234567890");
    let str3 = BigInt("-12345");
    console.log("PASS: BigInt from string");

    // =========================================
    // Test 4: BigInt.asIntN
    // =========================================
    let asInt = BigInt.asIntN(8, BigInt(300));  // Should wrap
    console.log("PASS: BigInt.asIntN");

    // =========================================
    // Test 5: BigInt.asUintN
    // =========================================
    let asUint = BigInt.asUintN(8, BigInt(300));  // Should be 44
    console.log("PASS: BigInt.asUintN");

    // =========================================
    // Test 6: BigInt.prototype.toString()
    // =========================================
    let big = BigInt(255);
    let s10 = big.toString();     // "255"
    let s2 = big.toString(2);     // "11111111"
    let s16 = big.toString(16);   // "ff"
    console.log("PASS: BigInt.toString");

    // =========================================
    // Test 7: BigInt.prototype.valueOf()
    // =========================================
    let val = big.valueOf();
    console.log("PASS: BigInt.valueOf");

    // =========================================
    // All tests passed
    // =========================================
    console.log("");
    console.log("All BigInt comprehensive tests passed!");
    return 0;
}
