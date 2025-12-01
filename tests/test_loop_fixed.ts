// Test file to verify that loops now work correctly after fixing type conversion

function testWhileLoop(): number {
    let i = 0;
    let count = 0;
    
    while (i < 5) {
        count = count + 1;
        i = i + 1;
    }
    
    return count;
}

function testForLoop(): number {
    let sum = 0;
    
    for (let i = 0; i < 3; i = i + 1) {
        sum = sum + i;
    }
    
    return sum;
}

function testDoWhileLoop(): number {
    let i = 0;
    let count = 0;
    
    do {
        count = count + 1;
        i = i + 1;
    } while (i < 3);
    
    return count;
}

function testBreak(): number {
    let i = 0;
    let count = 0;
    
    while (i < 10) {
        count = count + 1;
        if (count == 3) {
            break;
        }
        i = i + 1;
    }
    
    return count;
}

function testContinue(): number {
    let i = 0;
    let sum = 0;
    
    while (i < 5) {
        i = i + 1;
        if (i == 3) {
            continue;
        }
        sum = sum + i;
    }
    
    return sum;
}

function main(): number {
    let passed = 0;
    let total = 5;
    
    let test1 = testWhileLoop();
    if (test1 == 5) passed = passed + 1;
    
    let test2 = testForLoop();
    if (test2 == 3) passed = passed + 1;  // 0 + 1 + 2 = 3
    
    let test3 = testDoWhileLoop();
    if (test3 == 3) passed = passed + 1;
    
    let test4 = testBreak();
    if (test4 == 3) passed = passed + 1;
    
    let test5 = testContinue();
    if (test5 == 9) passed = passed + 1;  // 1 + 2 + 4 + 5 = 12 (3 is skipped)
    
    return passed;
}