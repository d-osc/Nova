// Test nested control flow: if/else inside loops
function testIfInLoop() {
    let i = 0;
    let result = 0;
    
    while (i < 5) {
        if (i % 2 == 0) {
            result = result + i;
        } else {
            result = result - i;
        }
        i = i + 1;
    }
    
    return result;
}

// Test loops inside if/else
function testLoopInIf() {
    let x = 10;
    let result = 0;
    
    if (x > 5) {
        let i = 0;
        while (i < 3) {
            result = result + x;
            i = i + 1;
        }
    } else {
        let i = 0;
        while (i < 2) {
            result = result + x;
            i = i + 1;
        }
    }
    
    return result;
}

// Test nested loops
function testNestedLoops() {
    let result = 0;
    let i = 0;
    
    while (i < 3) {
        let j = 0;
        while (j < 3) {
            result = result + i * j;
            j = j + 1;
        }
        i = i + 1;
    }
    
    return result;
}

// Test for loop with if/else
function testForWithIf() {
    let result = 0;
    
    for (let i = 0; i < 5; i = i + 1) {
        if (i < 2) {
            result = result + 1;
        } else if (i < 4) {
            result = result + 2;
        } else {
            result = result + 3;
        }
    }
    
    return result;
}

// Test do-while with if/else
function testDoWhileWithIf() {
    let x = 0;
    let result = 0;
    
    do {
        if (x > 2) {
            result = result + x;
        } else {
            result = result + 1;
        }
        x = x + 1;
    } while (x < 5);
    
    return result;
}

// Test complex nested structure
function testComplexNesting() {
    let result = 0;
    let i = 0;
    
    while (i < 3) {
        if (i % 2 == 0) {
            for (let j = 0; j < 2; j = j + 1) {
                if (j == 1) {
                    result = result + i * j;
                } else {
                    result = result + i + j;
                }
            }
        } else {
            let x = 0;
            do {
                result = result + x;
                x = x + 1;
            } while (x < 2);
        }
        i = i + 1;
    }
    
    return result;
}

// Main function to run all tests
function main() {
    let result = 0;
    
    result = result + testIfInLoop();
    result = result + testLoopInIf();
    result = result + testNestedLoops();
    result = result + testForWithIf();
    result = result + testDoWhileWithIf();
    result = result + testComplexNesting();
    
    return result;
}