function testForLoop(): number {
    let count = 0;
    
    for (let i = 0; i < 5; i = i + 1) {
        count = count + 1;
    }
    
    return count;
}

function testForWithSum(): number {
    let sum = 0;
    
    for (let i = 1; i <= 10; i = i + 1) {
        sum = sum + i;
    }
    
    return sum;
}

function testForNested(): number {
    let count = 0;
    
    for (let i = 0; i < 3; i = i + 1) {
        for (let j = 0; j < 3; j = j + 1) {
            count = count + 1;
        }
    }
    
    return count;
}

function testForWithIf(): number {
    let count = 0;
    
    for (let i = 0; i < 10; i = i + 1) {
        if (i % 2 === 0) {
            count = count + 1;
        }
    }
    
    return count;
}

function testForWithoutInit(): number {
    let i = 0;
    let count = 0;
    
    for (; i < 5; i = i + 1) {
        count = count + 1;
    }
    
    return count;
}

function testForWithoutCondition(): number {
    let count = 0;
    let i = 0;
    
    for (; i < 5; ) {
        if (i >= 3) {
            break;
        }
        count = count + 1;
        i = i + 1;
    }
    
    return count;
}

function main(): number {
    const result1 = testForLoop();
    const result2 = testForWithSum();
    const result3 = testForNested();
    const result4 = testForWithIf();
    const result5 = testForWithoutInit();
    const result6 = testForWithoutCondition();
    
    return result1 + result2 + result3 + result4 + result5 + result6;
}