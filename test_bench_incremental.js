console.log("=== Testing incrementally ===");

// 1. Loop
var sum = 0;
for (var i = 0; i < 10000000; i++) {
    sum += i;
}
console.log("1. Loop OK");

// 2. Math operations
var result = 0;
for (var i = 0; i < 1000000; i++) {
    result = Math.sqrt(i) + Math.pow(i, 2) % 1000;
}
console.log("2. Math OK");

// 3. Array operations
var arr = [];
for (var i = 0; i < 100000; i++) {
    arr.push(i);
}
console.log("3. Array OK");

// 4. String concatenation
var str = "";
for (var i = 0; i < 10000; i++) {
    str = str + "a";
}
console.log("4. String OK");

console.log("\nAll sections passed!");
