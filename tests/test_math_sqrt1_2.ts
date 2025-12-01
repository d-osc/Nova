// Test Math.SQRT1_2 constant (as integer, will be truncated to 0)
function main(): number {
    let sqrt1_2 = Math.SQRT1_2;
    return sqrt1_2;  // Should be 0 (truncated from 0.7071067811865476)
}
