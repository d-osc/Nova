// Test String.repeat()
function main(): number {
    let str = "AB";
    let repeated = str.repeat(3);  // Should be "ABABAB"

    // Verify: "ABABAB" -> A=65, B=66, A=65, B=66, A=65, B=66
    // Sum: 65+66+65+66+65+66 = 393
    let sum = 0;
    sum = sum + repeated.charAt(0);  // A = 65
    sum = sum + repeated.charAt(1);  // B = 66
    sum = sum + repeated.charAt(2);  // A = 65
    sum = sum + repeated.charAt(3);  // B = 66
    sum = sum + repeated.charAt(4);  // A = 65
    sum = sum + repeated.charAt(5);  // B = 66

    return sum;  // Should be 393
}
