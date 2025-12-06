// Test 1: Simple loop
console.log("Test 1: Loop");
var sum = 0;
for (var i = 0; i < 1000; i++) {
    sum += i;
}
console.log("Sum: " + sum);

// Test 2: Math operations
console.log("Test 2: Math");
var result = Math.sqrt(100) + Math.pow(2, 3);
console.log("Result: " + result);

// Test 3: Array push
console.log("Test 3: Array");
var arr = [];
for (var i = 0; i < 100; i++) {
    arr.push(i);
}
console.log("Array length: " + arr.length);

console.log("All tests passed");
