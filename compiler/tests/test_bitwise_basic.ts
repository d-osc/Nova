// Test basic bitwise operations
function main(): number {
    // Bitwise AND
    let a = 12 & 10;   // 1100 & 1010 = 1000 = 8
    
    // Bitwise OR
    let b = 12 | 10;   // 1100 | 1010 = 1110 = 14
    
    // Bitwise XOR
    let c = 12 ^ 10;   // 1100 ^ 1010 = 0110 = 6
    
    // Left shift
    let d = 3 << 2;    // 11 << 2 = 1100 = 12
    
    // Right shift
    let e = 12 >> 2;   // 1100 >> 2 = 11 = 3
    
    // Bitwise NOT
    let f = ~5;        // ~0101 = ...1010 = -6
    
    // Sum: 8 + 14 + 6 + 12 + 3 + (-6) = 37
    return a + b + c + d + e + f;
}
