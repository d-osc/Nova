function main(): number {
    // Array.prototype.findLastIndex() - find last index matching condition
    // Searches array from end to beginning (right to left)
    // Similar to findIndex() but returns the last match instead of first
    // ES2023 feature

    let arr = [10, 20, 30, 40, 50, 30, 60];
    //          0   1   2   3   4   5   6

    // Find last index of element greater than 25
    // Should find index 6 (value 60)
    let a = arr.findLastIndex((x) => x > 25);  // 6

    // Find last index of element equal to 30
    // Should find index 5 (last occurrence)
    let b = arr.findLastIndex((x) => x == 30); // 5

    // Find last index of element less than 35
    // Should find index 5 (value 30, last element < 35 from right)
    let c = arr.findLastIndex((x) => x < 35);  // 5

    // Result: 6 + 5 + 5 = 16
    return a + b + c;
}
