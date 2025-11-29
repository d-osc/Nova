// Test simplest ArrayBuffer functionality

function main(): number {
    // Test 1: Create ArrayBuffer
    let buffer = new ArrayBuffer(16);
    console.log("Created ArrayBuffer");

    // Test 2: Create Uint8Array with length (not buffer)
    let arr = new Uint8Array(8);
    console.log("Created Uint8Array");

    console.log("Done!");
    return 0;
}
