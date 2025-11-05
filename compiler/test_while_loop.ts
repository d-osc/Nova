function testWhileLoop(): number {
    let count = 0;
    let i = 0;
    
    while (i < 5) {
        count = count + 1;
        i = i + 1;
    }
    
    return count;
}

function testWhileWithCondition(): number {
    let sum = 0;
    let i = 1;
    
    while (i <= 10) {
        sum = sum + i;
        i = i + 1;
    }
    
    return sum;
}

function testNestedWhile(): number {
    let count = 0;
    let i = 0;
    
    while (i < 3) {
        let j = 0;
        while (j < 3) {
            count = count + 1;
            j = j + 1;
        }
        i = i + 1;
    }
    
    return count;
}

function testWhileWithIf(): number {
    let count = 0;
    let i = 0;
    
    while (i < 10) {
        if (i % 2 === 0) {
            count = count + 1;
        }
        i = i + 1;
    }
    
    return count;
}

function main(): number {
    const result1 = testWhileLoop();
    const result2 = testWhileWithCondition();
    const result3 = testNestedWhile();
    const result4 = testWhileWithIf();
    
    return result1 + result2 + result3 + result4;
}