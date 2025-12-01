// Test loop with complex expressions
function testLoopWithExpressions() {
    let i = 0;
    let result = 0;
    
    while (i < 5) {
        result = result + (i * 2);
        i = i + 1;
    }
    
    return result;
}

// Test loop with function calls in condition
function testLoopWithFunctionCalls() {
    let x = 10;
    let result = 0;
    let i = 0;
    
    function getValue() {
        return x;
    }
    
    while (i < getValue()) {
        result = result + i;
        i = i + 1;
    }
    
    return result;
}

// Test loop with variable updates
function testLoopWithVariableUpdates() {
    let x = 1;
    let y = 2;
    let result = 0;
    
    while (x < 10) {
        result = result + x * y;
        x = x + y;
        y = y + 1;
    }
    
    return result;
}

// Test for loop with complex init and update
function testComplexForLoop() {
    let result = 0;
    let j = 10;
    
    for (let i = 0; i < j; i = i + 1) {
        result = result + i + j;
        j = j - 1;
    }
    
    return result;
}

// Test loop with nested if-else and expressions
function testLoopWithNestedIfElse() {
    let i = 0;
    let result = 0;
    
    while (i < 10) {
        if (i % 2 == 0) {
            if (i == 4) {
                result = result + i * 2;
            } else {
                result = result + i;
            }
        } else {
            result = result + i * 3;
        }
        i = i + 1;
    }
    
    return result;
}

// Main function to run all tests
function main() {
    let result = 0;
    
    result = result + testLoopWithExpressions();
    result = result + testLoopWithFunctionCalls();
    result = result + testLoopWithVariableUpdates();
    result = result + testComplexForLoop();
    result = result + testLoopWithNestedIfElse();
    
    return result;
}