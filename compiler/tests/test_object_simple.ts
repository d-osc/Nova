// Simple object property access test
function testObjectProperty(): number {
    let obj = {x: 10, y: 20};
    return obj.x;  // Should return 10
}

function main(): number {
    return testObjectProperty();  // Should return 10
}
