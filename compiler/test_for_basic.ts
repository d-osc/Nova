function testBasicFor(): number {
    let i = 0;
    let count = 0;
    
    for ( ; i < 5; i = i + 1) {
        count = count + 1;
    }
    
    return count;
}

function main(): number {
    return testBasicFor();
}