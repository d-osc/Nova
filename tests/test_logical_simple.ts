// Minimal logical operation test
function testSimpleAnd(): number {
    let x: number = 5;
    let y: number = 10;

    let result: number = 0;
    if (x > 0 && y > 0) {
        result = 1;
    }
    return result;
}

function main(): number {
    return testSimpleAnd();
}
