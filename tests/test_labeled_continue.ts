// Test labeled continue
function testLabeledContinue(): number {
    let result: number = 0;

    outer: for (let i: number = 0; i < 3; i = i + 1) {
        for (let j: number = 0; j < 3; j = j + 1) {
            if (j == 1) {
                continue outer;  // Should skip rest of inner loop and continue outer
            }
            result = result + 1;
        }
    }

    return result;  // Should be 3 (only j=0 for each i=0,1,2)
}

function main(): number {
    return testLabeledContinue();
}
