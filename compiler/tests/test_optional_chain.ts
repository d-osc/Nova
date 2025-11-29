// Test optional chaining
function main(): number {
    let obj = { x: { y: 10 } };
    let val = obj?.x?.y;
    if (val != 10) return 1;
    return 0;
}
