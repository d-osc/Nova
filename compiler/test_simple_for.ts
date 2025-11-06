function testSimpleFor(): number {
    let count = 0;
    
    for (let i = 0; i < 5; i = i + 1) {
        count = count + 1;
    }
    
    return count;
}

function main(): number {
    return testSimpleFor();
}