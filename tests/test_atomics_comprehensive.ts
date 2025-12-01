// Comprehensive test for Atomics (ES2017)

function main(): number {
    let arr = new Int32Array(8);

    // =========================================
    // Test 1: Atomics.store and Atomics.load
    // =========================================
    Atomics.store(arr, 0, 123);
    if (Atomics.load(arr, 0) !== 123) {
        console.log("FAIL: store/load");
        return 1;
    }
    console.log("PASS: store/load");

    // =========================================
    // Test 2: Atomics.add
    // =========================================
    arr[1] = 50;
    let old = Atomics.add(arr, 1, 25);
    if (old !== 50 || arr[1] !== 75) {
        console.log("FAIL: add");
        return 2;
    }
    console.log("PASS: add");

    // =========================================
    // Test 3: Atomics.sub
    // =========================================
    arr[2] = 100;
    old = Atomics.sub(arr, 2, 35);
    if (old !== 100 || arr[2] !== 65) {
        console.log("FAIL: sub");
        return 3;
    }
    console.log("PASS: sub");

    // =========================================
    // Test 4: Atomics.and
    // =========================================
    arr[3] = 15;  // 0b1111
    old = Atomics.and(arr, 3, 10);  // 0b1010
    if (old !== 15 || arr[3] !== 10) {  // 0b1010 = 10
        console.log("FAIL: and");
        return 4;
    }
    console.log("PASS: and");

    // =========================================
    // Test 5: Atomics.or
    // =========================================
    arr[4] = 5;  // 0b0101
    old = Atomics.or(arr, 4, 10);  // 0b1010
    if (old !== 5 || arr[4] !== 15) {  // 0b1111 = 15
        console.log("FAIL: or");
        return 5;
    }
    console.log("PASS: or");

    // =========================================
    // Test 6: Atomics.xor
    // =========================================
    arr[5] = 15;  // 0b1111
    old = Atomics.xor(arr, 5, 10);  // 0b1010
    if (old !== 15 || arr[5] !== 5) {  // 0b0101 = 5
        console.log("FAIL: xor");
        return 6;
    }
    console.log("PASS: xor");

    // =========================================
    // Test 7: Atomics.exchange
    // =========================================
    arr[6] = 999;
    old = Atomics.exchange(arr, 6, 111);
    if (old !== 999 || arr[6] !== 111) {
        console.log("FAIL: exchange");
        return 7;
    }
    console.log("PASS: exchange");

    // =========================================
    // Test 8: Atomics.compareExchange (success)
    // =========================================
    arr[7] = 200;
    old = Atomics.compareExchange(arr, 7, 200, 300);
    if (old !== 200 || arr[7] !== 300) {
        console.log("FAIL: compareExchange success");
        return 8;
    }
    console.log("PASS: compareExchange success");

    // =========================================
    // Test 9: Atomics.compareExchange (failure)
    // =========================================
    // arr[7] is now 300
    old = Atomics.compareExchange(arr, 7, 999, 400);  // wrong expected value
    if (old !== 300 || arr[7] !== 300) {  // Should not change
        console.log("FAIL: compareExchange failure");
        return 9;
    }
    console.log("PASS: compareExchange failure");

    // =========================================
    // Test 10: Atomics.isLockFree
    // =========================================
    if (Atomics.isLockFree(3) !== 0) {  // 3 bytes is never lock-free
        console.log("FAIL: isLockFree");
        return 10;
    }
    console.log("PASS: isLockFree");

    console.log("");
    console.log("All Atomics comprehensive tests passed!");
    return 0;
}
