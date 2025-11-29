// Test labeled for loop with explicit main function
function testLabeled(): number {
    let result: number = 0;
    myLoop: for (let i: number = 0; i < 3; i = i + 1) {
        result = result + i;
    }
    return result;
}

function main(): number {
    return testLabeled();
}
