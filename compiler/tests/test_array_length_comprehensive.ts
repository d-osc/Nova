// Comprehensive test for array.length property
function main(): number {
    // Test 1: Initial length
    let arr = [10, 20, 30];
    let len1 = arr.length;  // Should be 3

    // Test 2: Length after push
    arr.push(40);
    let len2 = arr.length;  // Should be 4

    // Test 3: Length after pop
    arr.pop();
    let len3 = arr.length;  // Should be 3

    // Test 4: Length after shift
    arr.shift();
    let len4 = arr.length;  // Should be 2

    // Test 5: Length after unshift
    arr.unshift(5);
    let len5 = arr.length;  // Should be 3

    // Test 6: Empty array after multiple pops
    arr.pop();
    arr.pop();
    arr.pop();
    let len6 = arr.length;  // Should be 0

    // Return: 3 + 4 + 3 + 2 + 3 + 0 = 15
    return len1 + len2 + len3 + len4 + len5 + len6;
}
