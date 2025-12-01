// Test simple logical operations
function testAnd() {
    let a = 1;
    let b = 0;
    
    return a && b;
}

function testOr() {
    let a = 1;
    let b = 0;
    
    return a || b;
}

function main() {
    let result = 0;
    
    result = result + testAnd();
    result = result + testOr();
    
    return result;
}