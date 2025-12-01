// Test Math.clz32() function
function main(): number {
    // Math.clz32() counts leading zero bits in 32-bit representation
    // For simplicity in integer type system, we'll implement basic version
    // clz32(0) = 32 (all zeros)
    // clz32(1) = 31 (0...001)
    // clz32(4) = 29 (0...0100)
    let a = Math.clz32(1);      // 31 leading zeros
    let b = Math.clz32(4);      // 29 leading zeros
    let c = Math.clz32(0);      // 32 leading zeros (special case)

    // Result: 31 + 29 + 32 = 92
    return a + b + c;
}
