function testAnd(): number {
    let x: number = 5;
    let y: number = 10;
    if (x > 0 && y > 0) {
        return 1;
    }
    return 0;
}

function main(): number {
    let result: number = 0;
    
    result = testAnd();  // Assign result
    
    if (result !== 1) {
        return 100;  // Error
    }
    
    return 42;  // Success
}
