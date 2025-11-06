// Comprehensive Test Suite for Nova Compiler
// Tests all currently implemented features

// Test 1: Basic Arithmetic and Functions
function testBasicArithmetic(): number {
    let a = 10;
    let b = 20;
    let sum = a + b;
    return sum;
}

// Test 2: Comparison Operators
function testComparisons(): number {
    let result = 0;
    if (10 > 5) {
        result = 1;
    } else {
        result = 0;
    }
    return result;
}

// Test 3: Mutable Variables (let keyword)
function testMutableVariables(): number {
    let x = 5;
    x = x + 1;
    return x;
}

// Test 4: String Operations
function testStringOperations(): number {
    let str1 = "Hello";
    let str2 = "World";
    
    // String equality comparison
    if (str1 == str2) {
        return 0;
    } else {
        return 1; // Different strings
    }
}

// Test 5: Object Literals
function testObjectLiterals(): number {
    let config = {
        name: "Nova",
        version: "1.0",
        isActive: true
    };
    
    // Just test that object creation works
    return 1;
}

// Main test runner
function main(): number {
    let passed = 0;
    let total = 5;
    
    // Run all tests
    let test1 = testBasicArithmetic();
    if (test1 == 30) passed = passed + 1;
    
    let test2 = testComparisons();
    if (test2 == 1) passed = passed + 1;
    
    let test3 = testMutableVariables();
    if (test3 == 6) passed = passed + 1;
    
    let test4 = testStringOperations();
    if (test4 == 1) passed = passed + 1;
    
    let test5 = testObjectLiterals();
    if (test5 == 1) passed = passed + 1;
    
    return passed;
}