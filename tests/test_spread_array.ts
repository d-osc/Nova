// Test spread operator for arrays
function main(): number {
    let arr1 = [1, 2, 3];
    let arr2 = [...arr1, 4, 5];  // Should be [1, 2, 3, 4, 5]

    // Sum all elements
    let sum = arr2[0] + arr2[1] + arr2[2] + arr2[3] + arr2[4];
    return sum;  // Should be 15 (1+2+3+4+5)
}
