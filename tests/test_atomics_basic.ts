// Test Atomics basic operations (ES2017)

function main(): number {
    // Create Int32Array for atomic operations
    let arr = new Int32Array(4);

    // Test Atomics.store and Atomics.load
    Atomics.store(arr, 0, 42);
    let value = Atomics.load(arr, 0);
    console.log("Atomics.store/load:", value);
    if (value !== 42) return 1;

    // Test Atomics.add - returns old value
    arr[1] = 10;
    let oldValue = Atomics.add(arr, 1, 5);
    console.log("Atomics.add old value:", oldValue);
    console.log("Atomics.add new value:", arr[1]);
    if (oldValue !== 10) return 2;
    if (arr[1] !== 15) return 3;

    // Test Atomics.sub - returns old value
    arr[2] = 100;
    oldValue = Atomics.sub(arr, 2, 30);
    console.log("Atomics.sub old value:", oldValue);
    console.log("Atomics.sub new value:", arr[2]);
    if (oldValue !== 100) return 4;
    if (arr[2] !== 70) return 5;

    console.log("All basic Atomics tests passed!");
    return 0;
}
