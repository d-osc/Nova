// Test Loop Performance with Ultra Optimizations
console.log("=== Loop Performance Benchmarks ===");

// Test 1: Simple counting loop (Loop Rotation + LICM)
console.log("\n1. Simple counting loop (10M iterations):");
let count1 = 0;
for (let i = 0; i < 10000000; i++) {
    count1 = count1 + 1;
}
console.log("Count:", count1);

// Test 2: Array iteration (Loop optimizations + Array SIMD)
console.log("\n2. Array sum (1M elements):");
let arr = [];
for (let i = 0; i < 1000000; i++) {
    arr.push(i);
}
let sum = 0;
for (let i = 0; i < arr.length; i++) {
    sum = sum + arr[i];
}
console.log("Sum first 10:", arr[0] + arr[1] + arr[2] + arr[3] + arr[4]);

// Test 3: Nested loops (Loop invariant code motion)
console.log("\n3. Nested loops (1000x1000):");
let nested = 0;
for (let i = 0; i < 1000; i++) {
    for (let j = 0; j < 1000; j++) {
        nested = nested + 1;
    }
}
console.log("Nested count:", nested);

// Test 4: Loop with invariant calculation (LICM should move out)
console.log("\n4. Loop with invariant (1M iterations):");
let base = 42;
let result1 = 0;
for (let i = 0; i < 1000000; i++) {
    let temp = base * 2;  // Loop invariant - should be moved out
    result1 = result1 + temp;
}
console.log("Result:", result1);

// Test 5: Simple increment loop (Loop unrolling candidate)
console.log("\n5. Simple increment (10M iterations):");
let inc = 0;
for (let i = 0; i < 10000000; i++) {
    inc++;
}
console.log("Inc:", inc);

// Test 6: Array write loop (Loop rotation benefit)
console.log("\n6. Array write loop (100K elements):");
let arr2 = [];
for (let i = 0; i < 100000; i++) {
    arr2.push(i * 2);
}
console.log("Array filled, length:", arr2.length);
console.log("Sample values:", arr2[0], arr2[1000], arr2[99999]);

console.log("\n✓ All loop benchmarks completed!");
