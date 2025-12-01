function testBreak() {
    let i = 0;
    while (i < 10) {
        if (i == 5) {
            break;
        }
        i++;
    }
    return i;
}

function testContinue() {
    let i = 0;
    let sum = 0;
    while (i < 10) {
        if (i == 5) {
            i++;
            continue;
        }
        sum += i;
        i++;
    }
    return sum;
}

function main() {
    const result1 = testBreak();
    const result2 = testContinue();
    return result1 + result2;
}