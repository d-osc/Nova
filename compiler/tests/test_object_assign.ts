// Object property assignment test
function testObjectAssign(): number {
    let obj = {x: 10, y: 20};
    obj.x = 42;
    return obj.x;  // Should return 42
}

function main(): number {
    return testObjectAssign();
}
