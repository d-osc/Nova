// Test SharedArrayBuffer (ES2017)

function main(): number {
    // Create SharedArrayBuffer with 16 bytes
    let sab = new SharedArrayBuffer(16);

    // Check byteLength
    console.log("SharedArrayBuffer byteLength:", sab.byteLength);
    if (sab.byteLength !== 16) return 1;

    // Create Int32Array view over SharedArrayBuffer
    // This allows atomic operations on the shared buffer
    let int32View = new Int32Array(sab);

    // Set some values using Atomics
    Atomics.store(int32View, 0, 42);
    Atomics.store(int32View, 1, 100);
    Atomics.store(int32View, 2, 200);
    Atomics.store(int32View, 3, 300);

    // Read values back
    let v0 = Atomics.load(int32View, 0);
    let v1 = Atomics.load(int32View, 1);
    let v2 = Atomics.load(int32View, 2);
    let v3 = Atomics.load(int32View, 3);

    console.log("Values:", v0, v1, v2, v3);

    if (v0 !== 42) return 2;
    if (v1 !== 100) return 3;
    if (v2 !== 200) return 4;
    if (v3 !== 300) return 5;

    console.log("SharedArrayBuffer tests passed!");
    return 0;
}
