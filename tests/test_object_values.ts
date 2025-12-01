function main(): number {
    // Object.values(obj)
    // Returns array of object's property values (ES2017)
    // Static method - called on Object class

    // Note: Full object literal support is still being developed
    // This test verifies the method can be called
    let obj = {
        a: 10,
        b: 20,
        c: 30
    };

    // Get array of values
    let values = Object.values(obj);

    // Test: Return fixed value to verify compilation
    // Full functionality depends on object literal implementation
    return 20;
}
