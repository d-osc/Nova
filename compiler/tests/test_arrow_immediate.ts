// Test immediate invocation of arrow function (IIFE)
function main(): number {
    // This won't work yet because we need function pointers
    // But let's test that the arrow function compiles
    let result = ((a, b) => a + b)(5, 3);
    return result;
}
