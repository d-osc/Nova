function testBooleanBasics() {
    let a = true;
    let b = false;
    
    if (a) {
        return 1;
    } else {
        return 0;
    }
}

function testBooleanOperations() {
    let a = true;
    let b = false;
    
    let andResult = a && b;
    let orResult = a || b;
    let notResult = !a;
    
    if (andResult) {
        return 1;
    } else if (orResult) {
        return 2;
    } else if (notResult) {
        return 3;
    } else {
        return 4;
    }
}

function main() {
    return testBooleanBasics() + testBooleanOperations();
}