// Test labeled for loop inside a function
function testLabeled(): number {
    let result = 0;
    myLoop: for (let i = 0; i < 3; i++) {
        result = result + i;
    }
    return result;
}

let x = testLabeled();
console.log(x);
