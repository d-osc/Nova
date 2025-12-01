// Nested object property access test
function testNestedObj(): number {
    let obj = {x: 10, child: {value: 42}};
    return obj.child.value;  // Should return 42
}

function main(): number {
    return testNestedObj();
}
