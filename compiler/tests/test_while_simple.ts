// Simple while loop test
function testWhile(): number {
    let count: number = 0;
    while (count < 5) {
        count = count + 1;
    }
    return count;
}

function main(): number {
    return testWhile();  // Should return 5
}
