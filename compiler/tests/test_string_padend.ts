// Test String.padEnd()
function main(): number {
    let str = "5";
    let padded = str.padEnd(3, "0");
    return padded.charAt(2);  // Should be '0' = 48
}
