// Test String.prototype.localeCompare() - v1.3.60
function main(): number {
    let result = 0;

    // Test equal strings
    let a = "apple";
    let b = "apple";
    if (a.localeCompare(b) == 0) {
        result = result + 1;  // 1
    }

    // Test a < b (apple < banana)
    let c = "apple";
    let d = "banana";
    if (c.localeCompare(d) < 0) {
        result = result + 2;  // 3
    }

    // Test a > b (zebra > apple)
    let e = "zebra";
    let f = "apple";
    if (e.localeCompare(f) > 0) {
        result = result + 4;  // 7
    }

    // Test case sensitivity (A < a in ASCII)
    let g = "Apple";
    let h = "apple";
    if (g.localeCompare(h) < 0) {
        result = result + 8;  // 15
    }

    // Add base value for test identification
    return result + 200;  // Expected: 215 (15 + 200)
}
