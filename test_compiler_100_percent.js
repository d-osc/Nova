// Comprehensive test of all JavaScript features

console.log("=== Testing All Features ===");

// 1. Variables and primitives
const str = "Hello";
const num = 42;
const bool = true;
console.log("1. Primitives:", str, num, bool);

// 2. Arrays
const arr = [1, 2, 3];
arr.push(4);
console.log("2. Arrays:", arr.length);

// 3. Objects
const obj = { name: "Test", value: 123 };
console.log("3. Objects:", obj.name);

// 4. Functions
function add(a, b) {
    return a + b;
}
console.log("4. Functions:", add(10, 20));

// 5. Arrow functions
const multiply = (a, b) => a * b;
console.log("5. Arrow functions:", multiply(5, 6));

// 6. Classes
class Person {
    constructor(name) {
        this.name = name;
    }
    greet() {
        return this.name;
    }
}
const p = new Person("Alice");
console.log("6. Classes:", p.greet());

// 7. Inheritance
class Student extends Person {
    constructor(name, grade) {
        super(name);
        this.grade = grade;
    }
}
const s = new Student("Bob", 90);
console.log("7. Inheritance:", s.name, s.grade);

// 8. Template literals
const name = "World";
const greeting = `Hello ${name}`;
console.log("8. Template literals:", greeting);

// 9. Destructuring
const [a, b] = [1, 2];
console.log("9. Destructuring:", a, b);

// 10. Spread operator
const arr2 = [...arr, 5, 6];
console.log("10. Spread:", arr2.length);

// 11. Rest parameters
function sum(...numbers) {
    let total = 0;
    for (let n of numbers) {
        total = total + n;
    }
    return total;
}
console.log("11. Rest params:", sum(1, 2, 3, 4));

// 12. For-of loop
let count = 0;
for (let x of [1, 2, 3]) {
    count = count + x;
}
console.log("12. For-of:", count);

// 13. Ternary operator
const result = num > 40 ? "yes" : "no";
console.log("13. Ternary:", result);

// 14. Logical operators
const and = true && false;
const or = true || false;
console.log("14. Logical ops:", and, or);

// 15. Comparison operators
const eq = (42 === 42);
const neq = (42 !== 43);
console.log("15. Comparisons:", eq, neq);

console.log("=== All Tests Complete ===");
