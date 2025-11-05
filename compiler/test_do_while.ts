function testDoWhileLoop(): number {
    let count = 0;
    let i = 0;
    
    do {
        count = count + 1;
        i = i + 1;
    } while (i < 5);
    
    return count;
}

function testDoWhileAtLeastOnce(): number {
    let count = 0;
    let i = 10;
    
    do {
        count = count + 1;
    } while (i < 5);
    
    return count;
}

function testDoWhileWithCondition(): number {
    let sum = 0;
    let i = 1;
    
    do {
        sum = sum + i;
        i = i + 1;
    } while (i <= 5);
    
    return sum;
}

function main(): number {
    const result1 = testDoWhileLoop();
    const result2 = testDoWhileAtLeastOnce();
    const result3 = testDoWhileWithCondition();
    
    return result1 + result2 + result3;
}