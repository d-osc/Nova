// Test Math.LOG10E constant (as integer, will be truncated to 0)
function main(): number {
    let log10e = Math.LOG10E;
    return log10e;  // Should be 0 (truncated from 0.4342944819032518)
}
