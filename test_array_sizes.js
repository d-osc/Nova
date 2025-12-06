// Test different array sizes to find the limit
console.log("Testing different array sizes...");

console.log("\nSize 10:");
const arr10 = [];
for (let i = 0; i < 10; i++) {
    arr10.push(i);
}
console.log("Length: " + arr10.length);

console.log("\nSize 100:");
const arr100 = [];
for (let i = 0; i < 100; i++) {
    arr100.push(i);
}
console.log("Length: " + arr100.length);

console.log("\nSize 1000:");
const arr1000 = [];
for (let i = 0; i < 1000; i++) {
    arr1000.push(i);
}
console.log("Length: " + arr1000.length);

console.log("\nAll sizes tested!");
