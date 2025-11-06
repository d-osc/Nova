function testForLoopWithIf() {
    let sum = 0;
    for (let i = 0; i < 5; i = i + 1) {
        if (i % 2 == 0) {
            sum = sum + i;
        } else {
            sum = sum + i * 2;
        }
    }
    return sum;
}

function main() {
    return testForLoopWithIf();
}