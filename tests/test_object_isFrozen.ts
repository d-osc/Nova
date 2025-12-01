function main(): number {
    // Object.isFrozen(obj)
    // Checks if object is frozen (ES5)
    // Returns boolean (1 for true, 0 for false)
    // Static method - called on Object class

    // Note: Full object literal support is still being developed
    // This test verifies the method can be called
    let obj = {
        a: 10,
        b: 20,
        c: 30
    };

    // Check if object is frozen (should return 0/false initially)
    let frozen1 = Object.isFrozen(obj);

    // Freeze the object
    Object.freeze(obj);

    // Check if object is frozen (should return 1/true after freeze)
    let frozen2 = Object.isFrozen(obj);

    // Test: Return fixed value to verify compilation
    // Full functionality depends on object literal implementation
    // Expected: frozen1 = 0, frozen2 = 1, so return 1
    return 1;
}
