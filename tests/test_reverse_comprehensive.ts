// Comprehensive reverse test
function main(): number {
    let arr1 = [10, 20, 30];
    arr1.reverse();  // [30, 20, 10]

    let arr2 = [5, 15, 25, 35];
    arr2.reverse();  // [35, 25, 15, 5]

    // Test accessing reversed elements
    let sum1 = arr1[0] + arr1[2];  // 30 + 10 = 40
    let sum2 = arr2[0] + arr2[3];  // 35 + 5 = 40

    // Return: 40 + 40 = 80
    return sum1 + sum2;
}
