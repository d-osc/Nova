// Test Number.prototype.toLocaleString()

function main(): number {
    console.log("=== Number.prototype.toLocaleString() ===");

    // Test with integer
    let num1 = 1234567;
    let str1 = num1.toLocaleString();
    console.log("1234567.toLocaleString():", str1);

    // Test with decimal
    let num2 = 1234.56;
    let str2 = num2.toLocaleString();
    console.log("1234.56.toLocaleString():", str2);

    // Test with small number
    let num3 = 42;
    let str3 = num3.toLocaleString();
    console.log("42.toLocaleString():", str3);

    // Test with negative number
    let num4 = -9876543;
    let str4 = num4.toLocaleString();
    console.log("-9876543.toLocaleString():", str4);

    console.log("PASS: Number.prototype.toLocaleString()");
    return 0;
}
