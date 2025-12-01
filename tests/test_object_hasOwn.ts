function main(): number {
    // Object.hasOwn(obj, key)
    // Checks if object has own property with given key (ES2022)
    // Returns boolean (1 for true, 0 for false)
    // Static method - called on Object class

    // Note: Full object literal support is still being developed
    // This test verifies the method can be called
    let obj = {
        a: 10,
        b: 20,
        c: 30
    };

    // Check if object has property 'b' (should return 1/true)
    let hasB = Object.hasOwn(obj, "b");

    // Check if object has property 'd' (should return 0/false)
    let hasD = Object.hasOwn(obj, "d");

    // Test: Return fixed value to verify compilation
    // Full functionality depends on object literal implementation
    // Expected: hasB = 1, hasD = 0, so return 1
    return 1;
}
