// JSON Benchmark Tests

console.log("=== JSON.stringify Benchmarks ===");

// Benchmark 1: Numbers
console.log("1. Numbers:");
const num1 = 0;
const num2 = -42;
const num3 = 999999;
console.log(JSON.stringify(num1));
console.log(JSON.stringify(num2));
console.log(JSON.stringify(num3));

// Benchmark 2: Strings
console.log("2. Strings:");
const str1 = "Hello World";
const str2 = "Nova Compiler";
const str3 = "TypeScript";
console.log(JSON.stringify(str1));
console.log(JSON.stringify(str2));
console.log(JSON.stringify(str3));

// Benchmark 3: Booleans
console.log("3. Booleans:");
const bool1 = true;
const bool2 = false;
console.log(JSON.stringify(bool1));
console.log(JSON.stringify(bool2));

// Benchmark 4: Arrays - Small
console.log("4. Arrays - Small:");
const arr1 = [1, 2, 3];
console.log(JSON.stringify(arr1));

// Benchmark 5: Arrays - Medium
console.log("5. Arrays - Medium:");
const arr2 = [10, 20, 30, 40, 50];
console.log(JSON.stringify(arr2));

// Benchmark 6: Arrays - Large
console.log("6. Arrays - Large:");
const arr3 = [100, 200, 300, 400, 500, 600, 700, 800, 900, 1000];
console.log(JSON.stringify(arr3));

// Benchmark 7: Empty Array
console.log("7. Empty Array:");
const emptyArr = [];
console.log(JSON.stringify(emptyArr));

// Benchmark 8: Single Element Array
console.log("8. Single Element:");
const single = [42];
console.log(JSON.stringify(single));

console.log("=== All Tests Passed ===");
