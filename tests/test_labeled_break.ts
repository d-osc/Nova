// Test labeled break
function testLabeledBreak(): number {
    let result: number = 0;

    outer: for (let i: number = 0; i < 3; i = i + 1) {
        for (let j: number = 0; j < 3; j = j + 1) {
            if (i == 1 && j == 1) {
                break outer;  // Should break both loops
            }
            result = result + 1;
        }
    }

    return result;  // Should be 4 (i=0: j=0,1,2 = 3, i=1: j=0 = 1, then break)
}

function main(): number {
    return testLabeledBreak();
}
