function testComplexLoop() {
    let sum = 0;
    let product = 1;
    
    for (let i = 1; i <= 5; i = i + 1) {
        sum = sum + i;
        product = product * i;
        
        // คำนวณค่าเฉพาะใน loop
        let temp = sum + product;
        if (temp > 10) {
            sum = sum + 1;
        }
    }
    
    return sum + product;
}

function testNestedLoops() {
    let result = 0;
    
    for (let i = 0; i < 3; i = i + 1) {
        for (let j = 0; j < 3; j = j + 1) {
            result = result + (i * j);
        }
    }
    
    return result;
}

function main() {
    let a = testComplexLoop();
    let b = testNestedLoops();
    return a + b;
}