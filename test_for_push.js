// Test for loop with push
console.log("Test 1: Simple for loop");
for (let i = 0; i < 3; i++) {
    console.log("Loop " + i);
}

console.log("Test 2: Array creation + for loop push");
const arr = [];
for (let i = 0; i < 3; i++) {
    console.log("Before push " + i);
    arr.push(i);
    console.log("After push " + i);
}

console.log("Done! Array length: " + arr.length);
