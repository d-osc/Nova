// Test Math.LOG2E constant (as integer, will be truncated to 1)
function main(): number {
    let log2e = Math.LOG2E;
    return log2e;  // Should be 1 (truncated from 1.4426950408889634)
}
