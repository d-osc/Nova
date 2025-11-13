// Test simple variable assignment in if block
function testAssign(): number {
    let x: number = 5;
    let result: number = 0;

    if (x > 0) {
        result = 10;
    }

    return result;
}

function main(): number {
    return testAssign();  // Should return 10
}
