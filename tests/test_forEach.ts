// Test array.forEach() method
function main(): number {
    let arr = [1, 2, 3, 4, 5];
    let sum = 0;
    
    // forEach should call the callback for each element
    arr.forEach(function(value) {
        sum = sum + value;
    });
    
    return sum;  // Should be 15 (1+2+3+4+5)
}
