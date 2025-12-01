// Test Boolean() constructor function
function main(): number {
    // Boolean() returns 1 for truthy, 0 for falsy
    let a = Boolean(42);     // true (1) - non-zero
    let b = Boolean(0);      // false (0) - zero
    let c = Boolean(100);    // true (1) - non-zero
    let d = Boolean(-5);     // true (1) - non-zero

    // Result: 1 + 0 + 1 + 1 = 3
    return a + b + c + d;
}
