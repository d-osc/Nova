console.log("=== RUNTIME FUNCTIONS TEST ===");

// 1. Console functions
console.log("1. console.log: OK");

// 2. Array methods
const arr = [1, 2, 3];
arr.push(4);
const len = arr.length;
console.log("2. Array.push/length:", len);

// 3. String operations
const str1 = "Hello";
const str2 = "World";
const concat = str1 + " " + str2;
console.log("3. String concat:", concat);

// 4. String.length
const strLen = "Testing".length;
console.log("4. String.length:", strLen);

// 5. Math operations
const sum = 10 + 20;
const product = 5 * 6;
const division = 100 / 4;
console.log("5. Math ops:", sum, product, division);

// 6. Type checking
const num = 42;
const text = "test";
const flag = true;
console.log("6. Types:", num, text, flag);

// 7. Array indexing
const items = [10, 20, 30];
const first = items[0];
const last = items[2];
console.log("7. Array index:", first, last);

// 8. Template literals
const name = "Nova";
const msg = `Welcome to ${name}`;
console.log("8. Template:", msg);

// 9. Comparison
const isEqual = (5 === 5);
const isGreater = (10 > 5);
console.log("9. Comparison:", isEqual, isGreater);

// 10. Logical operators
const and = true && true;
const or = false || true;
console.log("10. Logical:", and, or);

console.log("=== BASIC RUNTIME COMPLETE ===");
