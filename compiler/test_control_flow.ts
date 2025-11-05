function testIfStatement(): number {
    if (5 > 3) {
        return 1;
    } else {
        return 0;
    }
}

function testIfElseChain(): number {
    const x = 10;
    if (x > 20) {
        return 3;
    } else if (x > 15) {
        return 2;
    } else if (x > 5) {
        return 1;
    } else {
        return 0;
    }
}

function testNestedIf(): number {
    const a = 5;
    const b = 10;
    
    if (a > 0) {
        if (b > a) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return -1;
    }
}

function testIfWithArithmetic(): number {
    const x = 3;
    const y = 4;
    
    if (x + y > 6) {
        return x * y;
    } else {
        return x + y;
    }
}

function main(): number {
    const result1 = testIfStatement();
    const result2 = testIfElseChain();
    const result3 = testNestedIf();
    const result4 = testIfWithArithmetic();
    
    return result1 + result2 + result3 + result4;
}