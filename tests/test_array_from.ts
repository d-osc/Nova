function main(): number {
    // Array.from(arrayLike)
    // Creates a new Array from an array-like or iterable object (ES2015)
    // Static method: called as Array.from(), not array.from()
    // For now, creates a shallow copy of an array

    let original = [10, 20, 30];
    // original = [10, 20, 30]

    // Create new array from original
    let copy = Array.from(original);
    // copy = [10, 20, 30]

    // Test: copy[1] = 20
    return copy[1];
}
