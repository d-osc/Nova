// Test String.slice()
function main(): number {
    let str = "Hello";
    let part = str.slice(1, 4);  // Should be "ell"

    // Verify: "ell" -> e=101, l=108, l=108
    // Sum: 101+108+108 = 317
    let sum = 0;
    sum = sum + part.charAt(0);  // e = 101
    sum = sum + part.charAt(1);  // l = 108
    sum = sum + part.charAt(2);  // l = 108

    return sum;  // Should be 317
}
