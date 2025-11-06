// Test break statement in while loop
function testBreakInWhile() {
    let i = 0;
    let result = 0;
    
    while (i < 10) {
        if (i == 5) {
            break;
        }
        result = result + i;
        i = i + 1;
    }
    
    return result;
}

// Test continue statement in while loop
function testContinueInWhile() {
    let i = 0;
    let result = 0;
    
    while (i < 5) {
        i = i + 1;
        if (i == 3) {
            continue;
        }
        result = result + i;
    }
    
    return result;
}

// Test break in for loop
function testBreakInFor() {
    let result = 0;
    
    for (let i = 0; i < 10; i = i + 1) {
        if (i == 3) {
            break;
        }
        result = result + i;
    }
    
    return result;
}

// Test continue in for loop
function testContinueInFor() {
    let result = 0;
    
    for (let i = 0; i < 5; i = i + 1) {
        if (i == 2) {
            continue;
        }
        result = result + i;
    }
    
    return result;
}

// Test break in do-while loop
function testBreakInDoWhile() {
    let i = 0;
    let result = 0;
    
    do {
        if (i == 3) {
            break;
        }
        result = result + i;
        i = i + 1;
    } while (i < 5);
    
    return result;
}

// Test continue in do-while loop
function testContinueInDoWhile() {
    let i = 0;
    let result = 0;
    
    do {
        i = i + 1;
        if (i == 2) {
            continue;
        }
        result = result + i;
    } while (i < 5);
    
    return result;
}

// Test nested loops with break and continue
function testNestedLoopsWithBreakContinue() {
    let result = 0;
    let i = 0;
    
    while (i < 3) {
        let j = 0;
        while (j < 3) {
            if (i == 1 && j == 1) {
                break;
            }
            if (j == 0) {
                j = j + 1;
                continue;
            }
            result = result + i * j;
            j = j + 1;
        }
        i = i + 1;
    }
    
    return result;
}

// Main function to run all tests
function main() {
    let result = 0;
    
    result = result + testBreakInWhile();
    result = result + testContinueInWhile();
    result = result + testBreakInFor();
    result = result + testContinueInFor();
    result = result + testBreakInDoWhile();
    result = result + testContinueInDoWhile();
    result = result + testNestedLoopsWithBreakContinue();
    
    return result;
}