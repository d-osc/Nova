function testBreakInWhile(): number {
    let i = 0;
    while (i < 10) {
        if (i == 5) {
            break;
        }
        i = i + 1;
    }
    return i;
}

function testContinueInWhile(): number {
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

function main(): number {
    return testBreakInWhile() + testContinueInWhile();
}