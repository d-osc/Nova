// Test BigInt prototype methods (ES2020)

function main(): number {
    // Test toString with default radix (10)
    let a = BigInt(255);
    let str10 = a.toString();
    console.log("255n.toString():", str10);

    // Test toString with radix 2 (binary)
    let str2 = a.toString(2);
    console.log("255n.toString(2):", str2);  // "11111111"

    // Test toString with radix 16 (hex)
    let str16 = a.toString(16);
    console.log("255n.toString(16):", str16);  // "ff"

    // Test valueOf
    let val = a.valueOf();
    console.log("255n.valueOf():", val);

    // Test with negative number
    let neg = BigInt(-100);
    let negStr = neg.toString();
    console.log("(-100n).toString():", negStr);

    // Test with large number
    let big = BigInt("123456789012345678901234567890");
    let bigStr = big.toString();
    console.log("Large BigInt toString:", bigStr);

    console.log("All BigInt method tests passed!");
    return 0;
}
