// Test all features that should work based on previous testing

console.log("=== Testing Working JavaScript Features ===");

// 1. Variables
console.log("\n1. Variables:");
const x = 42;
let y = 100;
console.log("const x =", x, "let y =", y);

// 2. Arrays
console.log("\n2. Arrays:");
const arr = [1, 2, 3];
console.log("Array:", arr[0], arr[1], arr[2]);
console.log("Array length:", arr.length);

// 3. Array push
console.log("\n3. Array push:");
const arr2 = [10];
arr2.push(20);
arr2.push(30);
console.log("After push:", arr2[0], arr2[1], arr2[2]);

// 4. Spread operator
console.log("\n4. Spread operator:");
const base = [1, 2];
const spread = [...base, 3, 4];
console.log("Spread result:", spread[0], spread[1], spread[2], spread[3]);

// 5. Object literals
console.log("\n5. Object literals:");
const obj = { a: 100, b: 200 };
console.log("obj.a =", obj.a, "obj.b =", obj.b);

// 6. Array destructuring
console.log("\n6. Array destructuring:");
const nums = [5, 10];
const [first, second] = nums;
console.log("Destructured:", first, second);

// 7. Functions
console.log("\n7. Functions:");
function add(a, b) {
    return a + b;
}
const result = add(5, 7);
console.log("add(5, 7) =", result);

// 8. Arrow functions
console.log("\n8. Arrow functions:");
const multiply = (a, b) => a * b;
const product = multiply(3, 4);
console.log("multiply(3, 4) =", product);

// 9. Classes
console.log("\n9. Classes:");
class Point {
    constructor(x, y) {
        this.x = x;
        this.y = y;
    }
    
    sum() {
        return this.x + this.y;
    }
}
const p = new Point(10, 20);
console.log("Point sum:", p.sum());

// 10. For loops
console.log("\n10. For loops:");
let sum = 0;
for (let i = 1; i <= 5; i++) {
    sum = sum + i;
}
console.log("Sum 1-5:", sum);

// 11. While loops
console.log("\n11. While loops:");
let count = 0;
let val = 1;
while (val <= 3) {
    count = count + val;
    val = val + 1;
}
console.log("While result:", count);

// 12. If/else
console.log("\n12. If/else:");
const num = 10;
if (num > 5) {
    console.log("Greater than 5");
} else {
    console.log("Not greater than 5");
}

// 13. Template literals
console.log("\n13. Template literals:");
const name = "Nova";
const version = 1;
console.log(`Compiler: ${name} v${version}`);

// 14. String methods
console.log("\n14. String methods:");
const str = "hello";
console.log("Length:", str.length);

// 15. Array map
console.log("\n15. Array map:");
const numbers = [1, 2, 3];
const doubled = numbers.map(n => n * 2);
console.log("Mapped:", doubled[0], doubled[1], doubled[2]);

console.log("\n=== All tests complete! ===");
