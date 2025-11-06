// Simple loop test
function testSimpleLoop() {
    let i = 0;
    let result = 0;
    
    while (i < 5) {
        result = result + i;
        i = i + 1;
    }
    
    return result;
}

function main() {
    return testSimpleLoop();
}