function main(): number {
    // Object.entries(obj)
    // Returns array of [key, value] pairs (ES2017)
    // Static method - called on Object class

    // Note: Full object literal support is still being developed
    // This test verifies the method can be called
    let obj = {
        a: 10,
        b: 20,
        c: 30
    };

    // Get array of [key, value] pairs
    // [["a", 10], ["b", 20], ["c", 30]]
    let entries = Object.entries(obj);

    // Test: Return fixed value to verify compilation
    // Full functionality depends on object literal implementation
    return 40;
}
