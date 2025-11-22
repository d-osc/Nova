// Simple test for array.reverse()
function main(): number {
    let arr = [10, 20, 30, 40, 50];
    arr.reverse();

    // After reverse: [50, 40, 30, 20, 10]
    // Return first element (should be 50)
    return arr[0];
}
