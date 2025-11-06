// Test basic arithmetic operations
function testArithmetic() {
    let a = 10;
    let b = 5;
    
    let sum = a + b;
    let diff = a - b;
    let product = a * b;
    let quotient = a / b;
    let remainder = a % b;
    
    return sum + diff + product + quotient + remainder;
}

// Test comparison operations
function testComparisons() {
    let a = 10;
    let b = 5;
    
    // Simplified comparison operations
    let equal = a == b;
    let notEqual = a != b;
    let lessThan = a < b;
    
    if (equal) {
        return 1;
    } else if (notEqual) {
        return 2;
    } else if (lessThan) {
        return 3;
    } else {
        return 0;
    }
}

// Test logical operations
function testLogical() {
    let a = 1;
    let b = 0;
    
    let andResult = a && b;
    let orResult = a || b;
    
    if (andResult) {
        return 1;
    } else if (orResult) {
        return 2;
    } else {
        return 0;
    }
}

// Test variable assignments
function testAssignments() {
    let x = 10;
    let y = x;
    let z = 5;
    
    x = x + 1;
    y = y * 2;
    z = z - 1;
    
    return x + y + z;
}

// Main function
function main() {
    let result = 0;
    
    result = result + testArithmetic();
    result = result + testComparisons();
    result = result + testLogical();
    result = result + testAssignments();
    
    return result;
}