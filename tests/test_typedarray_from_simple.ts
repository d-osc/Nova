// Simple test for TypedArray.from()

function main(): number {
    let arr = [10, 20, 30];
    let int32 = Int32Array.from(arr);

    console.log("Length:", int32.length);
    console.log("Element 0:", int32[0]);

    if (int32.length !== 3) return 1;
    if (int32[0] !== 10) return 2;

    console.log("Test passed!");
    return 0;
}
