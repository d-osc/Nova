// Test Array.prototype.includes() method
function main(): number {
    // Array.includes() checks if array contains a value
    let arr1 = [10, 20, 30, 40, 50];
    let arr2 = [5, 10, 15];

    let a = arr1.includes(30);      // 1 (true)
    let b = arr1.includes(99);      // 0 (false)
    let c = arr2.includes(10);      // 1 (true)
    let d = arr2.includes(20);      // 0 (false)
    let e = arr1.includes(50);      // 1 (true)

    // Result: 1 + 0 + 1 + 0 + 1 = 3
    return a + b + c + d + e;
}
