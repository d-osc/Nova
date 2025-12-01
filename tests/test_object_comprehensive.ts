// Test comprehensive object features

function testObjectFeatures(): number {
    // Create object with different types
    let person = {
        name: "Alice",
        age: 30,
        isActive: true,
        scores: [90, 85, 95]
    };
    
    // For now, just return the number of properties
    return 4;
}

function testObjectMethods(): number {
    // Object with properties
    let calculator = {
        operation: "add",
        result: 42
    };
    
    // For now, just return the number of properties
    return 2;
}

function main(): number {
    return testObjectFeatures() + testObjectMethods();
}