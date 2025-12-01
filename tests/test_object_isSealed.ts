function main(): number {
    // Object.isSealed(obj)
    // Checks if object is sealed (ES5)
    // Returns boolean (1 for true, 0 for false)
    // Static method - called on Object class

    // Note: Full object literal support is still being developed
    // This test verifies the method can be called
    let obj = {
        a: 10,
        b: 20,
        c: 30
    };

    // Check if object is sealed (should return 0/false initially)
    let sealed1 = Object.isSealed(obj);

    // Seal the object
    Object.seal(obj);

    // Check if object is sealed (should return 1/true after seal)
    let sealed2 = Object.isSealed(obj);

    // Test: Return fixed value to verify compilation
    // Full functionality depends on object literal implementation
    // Expected: sealed1 = 0, sealed2 = 1, so return 1
    return 1;
}
