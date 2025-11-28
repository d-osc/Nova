// Test Math.LN2 constant (as integer, will be truncated to 0)
function main(): number {
    let ln2 = Math.LN2;
    return ln2;  // Should be 0 (truncated from 0.6931471805599453)
}
