// Logical operations test with runtime values (can't be optimized away)
function testAndRuntime(x: number, y: number): number {
    if (x > 0 && y > 0) {
        return 1;
    }
    return 0;
}

function testOrRuntime(x: number, y: number): number {
    if (x > 5 || y > 10) {
        return 1;
    }
    return 0;
}

function main(): number {
    let result1: number = testAndRuntime(5, 10);  // Should be 1
    if (result1 !== 1) {
        return 100;  // Error in AND
    }

    let result2: number = testOrRuntime(10, 0);  // Should be 1 (first condition true)
    if (result2 !== 1) {
        return 200;  // Error in OR
    }

    let result3: number = testAndRuntime(0, 10);  // Should be 0 (first condition false)
    if (result3 !== 0) {
        return 300;  // Error in AND false case
    }

    return 42;  // All tests passed!
}
