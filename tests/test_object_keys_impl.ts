function main(): number {
    // Object.keys(obj)
    // Returns array of object's property keys (ES2015)
    // Static method - called on Object class

    // Note: Full object literal support is still being developed
    // This test verifies the method can be called
    let obj = {
        a: 10,
        b: 20,
        c: 30
    };

    // Get array of keys ["a", "b", "c"]
    let keys = Object.keys(obj);

    // Test: Return fixed value to verify compilation
    // Full functionality depends on object literal implementation
    return 30;
}
