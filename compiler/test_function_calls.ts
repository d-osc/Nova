// Test basic function calls
function add(a, b) {
    return a + b;
}

function multiply(a, b) {
    return a * b;
}

// Test function calls in expressions
function testFunctionCalls() {
    let x = 5;
    let y = 10;
    
    let result = add(x, y);
    result = result + multiply(x, y);
    
    return result;
}

// Test nested function calls
function testNestedFunctionCalls() {
    let a = 2;
    let b = 3;
    let c = 4;
    
    let result = add(add(a, b), c);
    result = add(result, multiply(a, b));
    
    return result;
}

// Test function calls with return values
function getValue() {
    return 42;
}

function testFunctionReturnValues() {
    let value = getValue();
    let result = value * 2;
    
    return result;
}

// Test recursive function calls
function factorial(n) {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

function testRecursiveFunction() {
    let result = factorial(5);
    return result;
}

// Main function
function main() {
    let result = 0;
    
    result = result + testFunctionCalls();
    result = result + testNestedFunctionCalls();
    result = result + testFunctionReturnValues();
    result = result + testRecursiveFunction();
    
    return result;
}