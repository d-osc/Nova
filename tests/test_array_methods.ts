// Test basic array methods
function main(): number {
    let arr = [10, 20, 30];
    
    // Test push - adds to end
    arr.push(40);  // arr = [10, 20, 30, 40]
    
    // Test pop - removes from end
    let last = arr.pop();  // last = 40, arr = [10, 20, 30]
    
    // Test shift - removes from front
    let first = arr.shift();  // first = 10, arr = [20, 30]
    
    // Test unshift - adds to front
    arr.unshift(5);  // arr = [5, 20, 30]
    
    // Result: last(40) + first(10) + arr[0](5) + arr[1](20) + arr[2](30)
    return last + first + arr[0] + arr[1] + arr[2];  // Should be 105
}
