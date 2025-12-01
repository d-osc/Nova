// Test Atomics bitwise operations (ES2017)

function main(): number {
    let arr = new Int32Array(4);

    // Test Atomics.and
    arr[0] = 0b1111;  // 15
    let oldValue = Atomics.and(arr, 0, 0b1010);  // AND with 10
    console.log("Atomics.and old value:", oldValue);
    console.log("Atomics.and new value:", arr[0]);
    if (oldValue !== 15) return 1;
    if (arr[0] !== 10) return 2;  // 0b1010 = 10

    // Test Atomics.or
    arr[1] = 0b0101;  // 5
    oldValue = Atomics.or(arr, 1, 0b1010);  // OR with 10
    console.log("Atomics.or old value:", oldValue);
    console.log("Atomics.or new value:", arr[1]);
    if (oldValue !== 5) return 3;
    if (arr[1] !== 15) return 4;  // 0b1111 = 15

    // Test Atomics.xor
    arr[2] = 0b1111;  // 15
    oldValue = Atomics.xor(arr, 2, 0b1010);  // XOR with 10
    console.log("Atomics.xor old value:", oldValue);
    console.log("Atomics.xor new value:", arr[2]);
    if (oldValue !== 15) return 5;
    if (arr[2] !== 5) return 6;  // 0b0101 = 5

    console.log("All Atomics bitwise tests passed!");
    return 0;
}
