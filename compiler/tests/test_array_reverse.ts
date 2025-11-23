// Test Array.prototype.reverse() method
function main(): number {
    // Array.reverse() reverses the array in-place
    let arr1 = [1, 2, 3, 4, 5];
    arr1.reverse();  // Now [5, 4, 3, 2, 1]

    let arr2 = [10, 20, 30];
    arr2.reverse();  // Now [30, 20, 10]

    // Result: arr1[0] + arr1[4] + arr2[0] + arr2[2]
    //       = 5 + 1 + 30 + 10 = 46
    return arr1[0] + arr1[4] + arr2[0] + arr2[2];
}
