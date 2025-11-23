// Test delete operator
function main(): number {
    let obj = { x: 42, y: 10 };

    // delete should remove a property
    delete obj.y;

    // After deletion, the property should not exist
    // For now, just test that delete compiles
    return obj.x;  // Should be 42
}
