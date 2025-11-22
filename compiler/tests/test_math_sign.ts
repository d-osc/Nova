// Test Math.sign function
function main(): number {
    let positive = Math.sign(42);   // Should be 1
    let negative = Math.sign(-30);  // Should be -1
    let zero = Math.sign(0);        // Should be 0
    
    return positive - negative + zero;  // Should be 1 - (-1) + 0 = 2
}
