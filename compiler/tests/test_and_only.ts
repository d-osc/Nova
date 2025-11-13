function testAnd(): number {
    let x: number = 5;
    let y: number = 10;

    // Simple AND - both true
    if (x > 0 && y > 0) {
        return 1;  // Should return this
    }
    return 0;
}

function main(): number {
    return testAnd();
}
