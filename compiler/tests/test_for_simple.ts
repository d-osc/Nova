// Simple for loop test
function testFor(): number {
    let sum: number = 0;
    for (let i: number = 0; i < 5; i = i + 1) {
        sum = sum + i;
    }
    return sum;
}

function main(): number {
    return testFor();  // Should return 10 (0+1+2+3+4)
}
