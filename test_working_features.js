// Working Features Test

console.log("=== WORKING FEATURES ===");

// 1. Variables
const x = 42;
let y = "test";
console.log("✓ Variables");

// 2. Primitives
const str = "Hello";
const num = 123;
const bool = true;
console.log("✓ Primitives");

// 3. Arrays
const arr = [1, 2, 3];
arr.push(4);
console.log("✓ Arrays");

// 4. Classes
class Animal {
    constructor(name) {
        this.name = name;
    }
    getName() {
        return this.name;
    }
}
console.log("✓ Classes");

// 5. Inheritance
class Dog extends Animal {
    constructor(name, breed) {
        super(name);
        this.breed = breed;
    }
}
const dog = new Dog("Max", "Golden");
console.log("✓ Inheritance:", dog.name, dog.breed);

// 6. Functions
function add(a, b) {
    return a + b;
}
console.log("✓ Functions:", add(10, 20));

// 7. Template literals
const name = "World";
const msg = `Hello ${name}`;
console.log("✓ Template literals:", msg);

// 8. For loops
let sum = 0;
for (let i = 0; i < 3; i = i + 1) {
    sum = sum + i;
}
console.log("✓ For loops:", sum);

// 9. While loops
let count = 0;
while (count < 3) {
    count = count + 1;
}
console.log("✓ While loops:", count);

// 10. If statements
const val = 10;
if (val > 5) {
    console.log("✓ If statements");
}

// 11. Ternary operator
const result = val > 5 ? "yes" : "no";
console.log("✓ Ternary:", result);

// 12. Comparison operators
const eq = (42 === 42);
console.log("✓ Comparisons:", eq);

// 13. Arithmetic
const arith = (10 + 5) * 2;
console.log("✓ Arithmetic:", arith);

// 14. String operations
const concat = "Hello" + " " + "World";
console.log("✓ String concat:", concat);

console.log("=== ALL WORKING ===");
