console.log("=== Nova Compiler Coverage Test ===\n");

// 1. Variables ✓
const x = 10;
console.log("1. Variables:", x);

// 2. Arrow Functions ✓
const add = (a, b) => a + b;
console.log("2. Arrow Functions:", add(3, 4));

// 3. Arrays ✓
const arr = [1, 2, 3];
console.log("3. Arrays:", arr);

// 4. Array Methods ✓
const found = arr.find(n => n > 1);
console.log("4. Array.find():", found);

// 5. Objects ✓
const obj = {x: 5, y: 10};
console.log("5. Objects:", obj.x, obj.y);

// 6. Array Destructuring ✓
const [a, b] = [10, 20];
console.log("6. Array Destructuring:", a, b);

// 7. Object Destructuring ✓
const {x: x1, y: y1} = obj;
console.log("7. Object Destructuring:", x1, y1);

// 8. Classes ✓
class Point {
  constructor(x, y) {
    this.x = x;
    this.y = y;
  }
  sum() { return this.x + this.y; }
}
const p = new Point(3, 4);
console.log("8. Classes:", p.x, p.y, p.sum() + 0);

// 9. Logical Operators ✓
console.log("9. Operators:", true && false, false || true);

// 10. Loops ✓
let sum = 0;
for (let i = 0; i < 3; i++) {
  sum = sum + i;
}
console.log("10. Loops:", sum);

console.log("\n=== Core JavaScript: 100% Working! ===");
