function testSimpleLoopBody() {
    let sum = 0;
    
    for (let i = 0; i < 3; i = i + 1) {
        sum = sum + i;
        sum = sum * 2;
    }
    
    return sum;
}

function main() {
    return testSimpleLoopBody();
}