// Test String.padStart()
function main(): number {
    let str = "5";
    let padded = str.padStart(3, "0");
    return padded.charAt(0);  // Should be '0' = 48
}
