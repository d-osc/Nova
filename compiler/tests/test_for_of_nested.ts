// Test nested for-of loops
function main(): number {
    let arr1 = [1, 2, 3];
    let arr2 = [10, 20];
    let sum = 0;

    for (let a of arr1) {
        for (let b of arr2) {
            sum = sum + (a * b);
        }
    }

    // Expected: (1*10 + 1*20) + (2*10 + 2*20) + (3*10 + 3*20)
    //         = (10 + 20) + (20 + 40) + (30 + 60)
    //         = 30 + 60 + 90 = 180
    return sum;
}
