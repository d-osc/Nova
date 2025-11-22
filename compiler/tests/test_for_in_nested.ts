// Test nested for-in loops
function main(): number {
    let arr1 = [10, 20];
    let arr2 = [1, 2, 3];
    let sum = 0;

    // Nested for-in: outer iterates indices of arr1, inner iterates indices of arr2
    for (let i in arr1) {
        for (let j in arr2) {
            // i will be 0, 1
            // j will be 0, 1, 2
            // Add the product of indices
            sum = sum + (i * j);
        }
    }

    // Expected: (0*0 + 0*1 + 0*2) + (1*0 + 1*1 + 1*2)
    //         = (0 + 0 + 0) + (0 + 1 + 2) = 3
    return sum;
}
