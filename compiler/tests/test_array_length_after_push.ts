// Test length after push
function main(): number {
    let arr = [10, 20, 30];
    arr.push(40);
    let len = arr.length;  // Should be 4
    return len;
}
