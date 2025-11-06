// Test object literal syntax

function testObjectLiteral(): number {
    // Create object literal
    let obj = { 
        name: "Nova",
        version: 1.0,
        isActive: true
    };
    
    // For now, just return the number of properties
    return 3;
}

function main(): number {
    return testObjectLiteral();
}