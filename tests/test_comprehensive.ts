// Test comprehensive Nova compiler features

// Function definitions
function add(a, b) {
    return a + b;
}

function multiply(a, b) {
    return a * b;
}

function factorial(n) {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

// Test arithmetic operations
function testArithmetic() {
    let x = 10;
    let y = 5;
    
    let sum = add(x, y);
    let product = multiply(x, y);
    
    return sum + product;
}

// Test while loop
function testWhileLoop() {
    let i = 0;
    let result = 0;
    
    while (i < 5) {
        result = result + i;
        i = i + 1;
    }
    
    return result;
}

// Test for loop
function testForLoop() {
    let result = 0;
    
    for (let i = 0; i < 3; i = i + 1) {
        result = result + i;
    }
    
    return result;
}

// Test do-while loop
function testDoWhileLoop() {
    let i = 0;
    let result = 0;
    
    do {
        result = result + i;
        i = i + 1;
    } while (i < 3);
    
    return result;
}

// Test nested control flow
function testNestedControlFlow() {
    let result = 0;
    let i = 0;
    
    while (i < 2) {
        if (i == 0) {
            let j = 0;
            while (j < 2) {
                result = result + i + j;
                j = j + 1;
            }
        } else {
            result = result + 10;
        }
        i = i + 1;
    }
    
    return result;
}

// Test function calls with loops
function testFunctionCallsWithLoops() {
    let result = 0;
    let i = 0;
    
    while (i < 3) {
        result = result + add(i, 1);
        i = i + 1;
    }
    
    return result;
}

// Test recursive function
function testRecursiveFunction() {
    return factorial(4);
}

// Main function to run all tests
function main() {
    let totalResult = 0;
    
    totalResult = totalResult + testArithmetic();
    totalResult = totalResult + testWhileLoop();
    totalResult = totalResult + testForLoop();
    totalResult = totalResult + testDoWhileLoop();
    totalResult = totalResult + testNestedControlFlow();
    totalResult = totalResult + testFunctionCallsWithLoops();
    totalResult = totalResult + testRecursiveFunction();
    
    return totalResult;
}