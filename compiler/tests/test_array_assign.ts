// Array element assignment test
function testArrayAssign(): number {
    let arr = [10, 20, 30];
    arr[0] = 42;
    return arr[0];  // Should return 42
}

function main(): number {
    return testArrayAssign();
}
