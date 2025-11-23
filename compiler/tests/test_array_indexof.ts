// Test Array.prototype.indexOf() method
function main(): number {
    // Array.indexOf() returns the first index of a value, or -1 if not found
    let arr = [10, 20, 30, 20, 40];

    let a = arr.indexOf(20);        // 1 (first occurrence)
    let b = arr.indexOf(30);        // 2
    let c = arr.indexOf(99);        // -1 (not found)
    let d = arr.indexOf(10);        // 0
    let e = arr.indexOf(40);        // 4

    // Result: 1 + 2 + (-1) + 0 + 4 = 6
    return a + b + c + d + e;
}
