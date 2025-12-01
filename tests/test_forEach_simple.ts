// Test array.forEach() with simple callback
function main(): number {
    let arr = [10, 20, 30];
    let sum = 0;

    // forEach calls the callback for each element
    arr.forEach((value: number) => {
        sum = sum + value;
    });

    return sum;  // Should be 60 (10 + 20 + 30)
}
