function main(): number {
    // Array.prototype.findLast() - find last element matching condition
    // Searches array from end to beginning (right to left)
    // Similar to find() but returns the last match instead of first
    // ES2023 feature

    let arr = [10, 20, 30, 40, 50, 30, 60];

    // Find last element greater than 25
    // Should find 60 (last element > 25, searching from right)
    let a = arr.findLast((x) => x > 25);  // 60

    // Find last element equal to 30
    // Should find 30 at index 5 (last occurrence)
    let b = arr.findLast((x) => x == 30); // 30

    // Find last element less than 35
    // Should find 30 at index 5 (last element < 35, searching from right)
    let c = arr.findLast((x) => x < 35);  // 30

    // Result: 60 + 30 + 30 = 120
    return a + b + c;
}
