// Test while loop
function testWhileLoop() {
    let count = 0;
    let sum = 0;
    
    while (count < 5) {
        sum = sum + count;
        count = count + 1;
    }
    
    return sum;
}

// Test for loop
function testForLoop() {
    let sum = 0;
    
    for (let i = 0; i < 5; i = i + 1) {
        sum = sum + i;
    }
    
    return sum;
}

// Test do-while loop
function testDoWhileLoop() {
    let count = 0;
    let sum = 0;
    
    do {
        sum = sum + count;
        count = count + 1;
    } while (count < 5);
    
    return sum;
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

// Test loop with if statement
function testLoopWithIf() {
    let i = 0;
    let result = 0;
    
    while (i < 10) {
        if (i < 5) {
            result = result + i;
        } else {
            result = result + i * 2;
        }
        i = i + 1;
    }
    
    return result;
}

// Main function
function main() {
    let result = 0;
    
    result = result + testWhileLoop();
    result = result + testForLoop();
    result = result + testDoWhileLoop();
    result = result + testNestedLoops();
    result = result + testLoopWithIf();
    
    return result;
}