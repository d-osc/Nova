// Simple test for TypedArray.of()

function main(): number {
    let int32 = Int32Array.of(10, 20, 30, 40);

    console.log("Length:", int32.length);
    console.log("Element 0:", int32[0]);

    if (int32.length !== 4) return 1;
    if (int32[0] !== 10) return 2;
    if (int32[1] !== 20) return 3;
    if (int32[3] !== 40) return 4;

    console.log("Test passed!");
    return 0;
}
