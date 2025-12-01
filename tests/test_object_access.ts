// Test object property access

function testObjectAccess(): number {
    // Create object literal
    let obj = { 
        name: "Nova",
        version: 1.0,
        count: 42
    };
    
    // Access object property
    // For now, just return the property count as placeholder
    return obj.count;
}

function main(): number {
    return testObjectAccess();
}