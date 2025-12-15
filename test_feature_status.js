// Comprehensive feature test for Nova compiler

console.log("=== FEATURE TEST SUITE ===");

// 1. Basic Variables
const x = 42;
const name = "Nova";
console.log("1. Variables: x =", x);

// 2. Arrow Functions
const double = (n) => n * 2;
const result = double(21);
console.log("2. Arrow functions: double(21) =", result);

// 3. Destructuring
const obj = {a: 10, b: 20};
const {a, b} = obj;
console.log("3. Destructuring: a =", a, "b =", b);

// 4. For-of loops
const nums = [1, 2, 3];
console.log("4. For-of loop:");
for (const n of nums) {
    console.log("  -", n);
}

// 5. Classes
class Animal {
    constructor(name) {
        this.name = name;
    }
    speak() {
        console.log(this.name, "makes a sound");
    }
}
const dog = new Animal("Dog");
console.log("5. Classes:");
dog.speak();

// 6. String methods
const str = "hello";
console.log("6. String length:", str.length);

// 7. Array methods (without spread)
const arr = [10, 20, 30];
console.log("7. Array length:", arr.length);

console.log("=== TEST COMPLETE ===");
