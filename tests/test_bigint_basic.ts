// Test BigInt basic operations (ES2020)

function main(): number {
    // Test BigInt constructor from number
    let a = BigInt(42);
    console.log("BigInt(42) created");

    // Test BigInt constructor from string
    let b = BigInt("123456789012345678901234567890");
    console.log("BigInt from large string created");

    // Test BigInt literal syntax
    let c = 999n;
    console.log("BigInt literal 999n created");

    // Test BigInt literal with large number
    let d = 12345678901234567890n;
    console.log("BigInt large literal created");

    // Test BigInt(0)
    let zero = BigInt(0);
    console.log("BigInt(0) created");

    // Test negative BigInt
    let neg = BigInt(-100);
    console.log("BigInt(-100) created");

    console.log("All BigInt basic tests passed!");
    return 0;
}
