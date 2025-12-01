// Direct test - just return the OR result
function testOr(x: number, y: number): number {
    if (x > 5 || y > 10) {
        return 3;
    }
    return 0;
}

function main(): number {
    return testOr(10, 0);  // 10 > 5 is true, so should return 3
}
