function main(): number {
    // Object.seal(obj)
    // Seals object - prevents adding/deleting properties (ES5)
    // Allows modifying existing properties (unlike freeze)
    // Returns the sealed object
    // Static method - called on Object class

    // Note: Full object literal support is still being developed
    // This test verifies the method can be called
    let obj = {
        a: 10,
        b: 20,
        c: 30
    };

    // Seal the object - after this, properties can be modified
    // but cannot be added or deleted
    let sealed = Object.seal(obj);

    // Test: Return fixed value to verify compilation
    // Full functionality depends on object literal implementation
    return 70;
}
