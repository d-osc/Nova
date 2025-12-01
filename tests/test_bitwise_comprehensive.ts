// Comprehensive bitwise operations test
function main(): number {
    // Test combinations
    let mask = 15;      // 0x0F = 0000 1111
    let value = 170;    // 0xAA = 1010 1010
    
    // Extract lower nibble
    let lower = value & mask;  // 1010 1010 & 0000 1111 = 0000 1010 = 10
    
    // Set all bits in mask region
    let filled = value | mask; // 1010 1010 | 0000 1111 = 1010 1111 = 175
    
    // Toggle mask bits
    let toggled = value ^ mask; // 1010 1010 ^ 0000 1111 = 1010 0101 = 165
    
    // Multiply by shifting
    let doubled = 7 << 1;  // 7 * 2 = 14
    let quad = 3 << 2;     // 3 * 4 = 12
    
    // Divide by shifting
    let halved = 20 >> 1;  // 20 / 2 = 10
    
    // Invert bits
    let inverted = ~0;  // -1
    
    // Sum: 10 + 175 + 165 + 14 + 12 + 10 + (-1) = 385
    return lower + filled + toggled + doubled + quad + halved + inverted;
}
