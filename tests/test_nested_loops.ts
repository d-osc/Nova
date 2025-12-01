function testNestedBreak() {
    let i = 0;
    let j = 0;
    while (i < 5) {
        while (j < 5) {
            if (i == 2 && j == 2) {
                break; // Should break out of inner loop only
            }
            j++;
        }
        i++;
    }
    return i + j;
}

function testNestedContinue() {
    let i = 0;
    let sum = 0;
    while (i < 5) {
        let j = 0;
        while (j < 5) {
            if (i == 2 && j == 2) {
                j++;
                continue; // Should continue with inner loop
            }
            sum += i + j;
            j++;
        }
        i++;
    }
    return sum;
}

function testBreakFromOuter() {
    let i = 0;
    let j = 0;
    let found = false;
    while (i < 5) {
        while (j < 5) {
            if (i == 2 && j == 2) {
                found = true;
                break; // Inner break
            }
            j++;
        }
        if (found) {
            break; // Outer break
        }
        i++;
    }
    return i + j;
}

function main() {
    const result1 = testNestedBreak();
    const result2 = testNestedContinue();
    const result3 = testBreakFromOuter();
    return result1 + result2 + result3;
}