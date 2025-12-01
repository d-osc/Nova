// Test spread syntax
function main(): number {
    let arr1 = [1, 2, 3];
    let arr2 = [...arr1, 4, 5];
    if (arr2.length != 5) return 1;
    return 0;
}
