function testSimpleBreak() {
    let i = 0;
    while (i < 10) {
        if (i == 3) {
            break;
        }
        i = i + 1;
    }
    return i;
}

function testSimpleContinue() {
    let i = 0;
    let sum = 0;
    while (i < 5) {
        i = i + 1;
        if (i == 3) {
            continue;
        }
        sum = sum + i;
    }
    return sum;
}

function main() {
    return testSimpleBreak() + testSimpleContinue();
}