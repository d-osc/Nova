// Direct test - just return the AND result
function testAnd(x: number, y: number): number {
    if (x > 0 && y > 0) {
        return 1;
    }
    return 0;
}

function main(): number {
    return testAnd(5, 10);
}
