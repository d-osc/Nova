// Test logical operations - AND (&&) and OR (||)
// Should test short-circuit behavior

function testAnd(): number {
    let x: number = 5;
    let y: number = 10;

    // Simple AND - both true
    if (x > 0 && y > 0) {
        return 1;  // Should return this
    }
    return 0;
}

function testAndShortCircuit(): number {
    let x: number = 0;
    let y: number = 10;

    // AND with first false - should NOT evaluate second
    // (In true short-circuit, y comparison should not happen)
    if (x > 5 && y > 0) {
        return 1;
    }
    return 2;  // Should return this
}

function testOr(): number {
    let x: number = 5;
    let y: number = 0;

    // Simple OR - first true
    if (x > 0 || y > 10) {
        return 3;  // Should return this
    }
    return 0;
}

function testOrShortCircuit(): number {
    let x: number = 10;
    let y: number = 0;

    // OR with first true - should NOT evaluate second
    // (In true short-circuit, y comparison should not happen)
    if (x > 5 || y > 10) {
        return 4;  // Should return this
    }
    return 0;
}

function main(): number {
    let result: number = 0;

    result = testAnd();  // Should be 1
    if (result !== 1) return 100;  // Error

    result = testAndShortCircuit();  // Should be 2
    if (result !== 2) return 200;  // Error

    result = testOr();  // Should be 3
    if (result !== 3) return 300;  // Error

    result = testOrShortCircuit();  // Should be 4
    if (result !== 4) return 400;  // Error

    return 42;  // All tests passed!
}
