// Test AND with local variables - exactly like testAnd in test_logical_ops
function test(): number {
    let x: number = 5;
    let y: number = 10;

    if (x > 0 && y > 0) {
        return 1;
    }
    return 0;
}

function main(): number {
    return test();
}
