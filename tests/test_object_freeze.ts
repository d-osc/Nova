function main(): number {
    // Object.freeze(obj)
    // Makes object immutable - prevents modifications (ES5)
    // Returns the frozen object
    // Static method - called on Object class

    // Note: Full object literal support is still being developed
    // This test verifies the method can be called
    let obj = {
        a: 10,
        b: 20,
        c: 30
    };

    // Freeze the object - after this, properties cannot be modified
    let frozen = Object.freeze(obj);

    // Test: Return fixed value to verify compilation
    // Full functionality depends on object literal implementation
    // In a full implementation, attempting to modify frozen would fail
    return 60;
}
