// Test basic while loop without break/continue
function testBasicWhile() {
    let i = 0;
    let result = 0;
    
    while (i < 3) {
        result = result + i;
        i = i + 1;
    }
    
    return result;
}

// Test nested while loops without break/continue
function testNestedWhile() {
    let result = 0;
    let i = 0;
    
    while (i < 2) {
        let j = 0;
        while (j < 2) {
            result = result + i + j;
            j = j + 1;
        }
        i = i + 1;
    }
    
    return result;
}

// Test while loop with if statement
function testWhileWithIf() {
    let i = 0;
    let result = 0;
    
    while (i < 5) {
        if (i == 3) {
            result = result + 10;
        } else {
            result = result + 1;
        }
        i = i + 1;
    }
    
    return result;
}

// Test basic for loop
function testBasicFor() {
    let result = 0;
    
    for (let i = 0; i < 3; i = i + 1) {
        result = result + i;
    }
    
    return result;
}

// Test nested for loops
function testNestedFor() {
    let result = 0;
    
    for (let i = 0; i < 2; i = i + 1) {
        for (let j = 0; j < 2; j = j + 1) {
            result = result + i + j;
        }
    }
    
    return result;
}

// Test basic do-while loop
function testBasicDoWhile() {
    let i = 0;
    let result = 0;
    
    do {
        result = result + i;
        i = i + 1;
    } while (i < 3);
    
    return result;
}

// Main function to run all tests
function main() {
    let result = 0;
    
    result = result + testBasicWhile();
    result = result + testNestedWhile();
    result = result + testWhileWithIf();
    result = result + testBasicFor();
    result = result + testNestedFor();
    result = result + testBasicDoWhile();
    
    return result;
}