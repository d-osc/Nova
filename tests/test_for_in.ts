// Test for...in loop
function main(): number {
    let obj = { x: 10, y: 20, z: 30 };
    let count = 0;

    // for...in should iterate over object keys
    for (let key in obj) {
        count = count + 1;
    }

    return count;  // Should be 3 (number of keys)
}
