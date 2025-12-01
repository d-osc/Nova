// Test Atomics exchange and compareExchange (ES2017)

function main(): number {
    let arr = new Int32Array(4);

    // Test Atomics.exchange - atomically swap value
    arr[0] = 100;
    let oldValue = Atomics.exchange(arr, 0, 200);
    console.log("Atomics.exchange old value:", oldValue);
    console.log("Atomics.exchange new value:", arr[0]);
    if (oldValue !== 100) return 1;
    if (arr[0] !== 200) return 2;

    // Test Atomics.compareExchange - successful swap
    arr[1] = 50;
    oldValue = Atomics.compareExchange(arr, 1, 50, 75);  // expected=50, replacement=75
    console.log("Atomics.compareExchange (success) old value:", oldValue);
    console.log("Atomics.compareExchange (success) new value:", arr[1]);
    if (oldValue !== 50) return 3;
    if (arr[1] !== 75) return 4;  // Should be replaced

    // Test Atomics.compareExchange - failed swap (expected != actual)
    arr[2] = 100;
    oldValue = Atomics.compareExchange(arr, 2, 999, 200);  // expected=999 (wrong), replacement=200
    console.log("Atomics.compareExchange (fail) old value:", oldValue);
    console.log("Atomics.compareExchange (fail) value unchanged:", arr[2]);
    if (oldValue !== 100) return 5;  // Returns actual old value
    if (arr[2] !== 100) return 6;  // Should NOT be replaced

    console.log("All Atomics exchange tests passed!");
    return 0;
}
