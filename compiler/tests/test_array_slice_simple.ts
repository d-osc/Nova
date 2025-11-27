// Test array slice - simple version
function main(): number {
    let arr = [10, 20, 30, 40, 50];
    arr = arr.slice(1, 4);  // Should be [20, 30, 40]
    
    // Access array directly after slice (inline)
    return arr.length;  // Should be 3
}
