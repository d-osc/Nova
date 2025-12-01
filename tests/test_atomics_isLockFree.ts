// Test Atomics.isLockFree (ES2017)

function main(): number {
    // Test Atomics.isLockFree for different byte sizes
    // Returns true (1) if atomic operations of the given byte size are lock-free

    let size1 = Atomics.isLockFree(1);  // 1-byte (int8)
    let size2 = Atomics.isLockFree(2);  // 2-byte (int16)
    let size4 = Atomics.isLockFree(4);  // 4-byte (int32) - most common
    let size8 = Atomics.isLockFree(8);  // 8-byte (int64)
    let size3 = Atomics.isLockFree(3);  // Invalid size - should return false

    console.log("Atomics.isLockFree(1):", size1);
    console.log("Atomics.isLockFree(2):", size2);
    console.log("Atomics.isLockFree(4):", size4);
    console.log("Atomics.isLockFree(8):", size8);
    console.log("Atomics.isLockFree(3):", size3);

    // size4 should almost always be lock-free on modern platforms
    if (size4 !== 1) {
        console.log("Warning: 4-byte atomics not lock-free");
    }

    // size3 should never be lock-free (not a valid atomic size)
    if (size3 !== 0) return 1;

    console.log("Atomics.isLockFree tests passed!");
    return 0;
}
