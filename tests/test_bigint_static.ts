// Test BigInt static methods (ES2020)

function main(): number {
    // Test BigInt.asIntN - wraps to signed integer
    let a = BigInt(300);
    let signed8 = BigInt.asIntN(8, a);  // 8-bit signed: -256 to 255
    console.log("BigInt.asIntN(8, 300):", signed8);  // Should wrap

    // Test BigInt.asUintN - wraps to unsigned integer
    let unsigned8 = BigInt.asUintN(8, a);  // 8-bit unsigned: 0 to 255
    console.log("BigInt.asUintN(8, 300):", unsigned8);  // Should be 44 (300 % 256)

    // Test with negative number
    let neg = BigInt(-10);
    let unsignedNeg = BigInt.asUintN(8, neg);
    console.log("BigInt.asUintN(8, -10):", unsignedNeg);  // Should be 246

    // Test 64-bit wrapping
    let big = BigInt("9223372036854775808");  // 2^63
    let signed64 = BigInt.asIntN(64, big);
    console.log("BigInt.asIntN(64, 2^63):", signed64);  // Should be negative

    console.log("All BigInt static method tests passed!");
    return 0;
}
