// Final Performance Test - 5000 iterations
const iterations = 5000;

for (let i = 0; i < iterations; i++) {
    const r1 = JSON.stringify(42);
    const r2 = JSON.stringify("test");
    const r3 = JSON.stringify(true);
    const arr = [1, 2, 3, 4, 5];
    const r4 = JSON.stringify(arr);
}
console.log("Complete: 5000 iterations");
