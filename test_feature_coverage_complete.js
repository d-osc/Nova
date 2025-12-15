// Comprehensive JavaScript feature coverage test
console.log("=== JavaScript Feature Coverage Test ===\n");

// 1. Variables and Constants
console.log("1. Variables and Constants");
let x = 10;
const y = 20;
var z = 30;
console.log("  let, const, var:", x, y, z);

// 2. Arithmetic Operations
console.log("\n2. Arithmetic Operations");
console.log("  Addition:", 5 + 3);
console.log("  Subtraction:", 10 - 4);
console.log("  Multiplication:", 6 * 7);
console.log("  Division:", 20 / 4);
console.log("  Modulo:", 17 % 5);
console.log("  Exponentiation:", 2 ** 3);

// 3. Comparison Operations
console.log("\n3. Comparison Operations");
console.log("  5 == 5:", 5 == 5);
console.log("  5 != 3:", 5 != 3);
console.log("  5 > 3:", 5 > 3);
console.log("  5 < 10:", 5 < 10);
console.log("  5 >= 5:", 5 >= 5);
console.log("  5 <= 5:", 5 <= 5);

// 4. Logical Operations
console.log("\n4. Logical Operations");
console.log("  true && true:", true && true);
console.log("  true || false:", true || false);
console.log("  !false:", !false);

// 5. String Operations
console.log("\n5. String Operations");
const str1 = "Hello";
const str2 = "World";
console.log("  Concatenation:", str1 + " " + str2);
console.log("  Length:", str1.length);
console.log("  Template literal:", `${str1} ${str2}`);

// 6. Array Operations
console.log("\n6. Array Operations");
const arr = [1, 2, 3, 4, 5];
console.log("  Array literal:", arr);
console.log("  Length:", arr.length);
console.log("  Access:", arr[0]);
arr.push(6);
console.log("  After push:", arr.length);
const popped = arr.pop();
console.log("  Popped value:", popped);

// 7. Array Methods
console.log("\n7. Array Methods");
const nums = [1, 2, 3, 4, 5];
const doubled = nums.map(x => x * 2);
console.log("  map:", doubled);
const filtered = nums.filter(x => x > 2);
console.log("  filter:", filtered);

// 8. Object Literals
console.log("\n8. Object Literals");
const obj = { name: "Alice", age: 30 };
console.log("  Object:", obj);
console.log("  Property access:", obj.name);

// 9. Functions
console.log("\n9. Functions");
function add(a, b) {
    return a + b;
}
console.log("  Function call:", add(5, 3));

// 10. Arrow Functions
console.log("\n10. Arrow Functions");
const multiply = (a, b) => a * b;
console.log("  Arrow function:", multiply(4, 5));

// 11. Nested Function Calls
console.log("\n11. Nested Function Calls");
function increment(x) { return x + 1; }
function double(x) { return x * 2; }
console.log("  Nested calls:", double(increment(5)));

// 12. Closures
console.log("\n12. Closures");
function makeCounter() {
    let count = 0;
    return function() {
        count++;
        return count;
    };
}
const counter = makeCounter();
console.log("  First call:", counter());
console.log("  Second call:", counter());

// 13. Classes
console.log("\n13. Classes");
class Person {
    constructor(name, age) {
        this.name = name;
        this.age = age;
    }

    greet() {
        return `Hello, I'm ${this.name}`;
    }
}
const person = new Person("Bob", 25);
console.log("  Class instance:", person.greet());

// 14. Class Inheritance
console.log("\n14. Class Inheritance");
class Animal {
    constructor(name) {
        this.name = name;
    }
    speak() {
        return `${this.name} makes a sound`;
    }
}
class Dog extends Animal {
    constructor(name, breed) {
        super(name);
        this.breed = breed;
    }
    speak() {
        return `${this.name} barks`;
    }
}
const dog = new Dog("Rex", "Labrador");
console.log("  Inheritance:", dog.speak());

// 15. Conditionals
console.log("\n15. Conditionals");
const num = 10;
if (num > 5) {
    console.log("  if statement: num > 5");
} else {
    console.log("  if statement: num <= 5");
}

// 16. Ternary Operator
console.log("\n16. Ternary Operator");
const result = num > 5 ? "greater" : "smaller";
console.log("  Ternary:", result);

// 17. Switch Statement
console.log("\n17. Switch Statement");
const day = 2;
switch (day) {
    case 1:
        console.log("  Monday");
        break;
    case 2:
        console.log("  Tuesday");
        break;
    default:
        console.log("  Other day");
}

// 18. For Loop
console.log("\n18. For Loop");
for (let i = 0; i < 3; i++) {
    console.log("  Loop iteration:", i);
}

// 19. While Loop
console.log("\n19. While Loop");
let i = 0;
while (i < 3) {
    console.log("  While iteration:", i);
    i++;
}

// 20. For-of Loop
console.log("\n20. For-of Loop");
const items = [10, 20, 30];
for (const item of items) {
    console.log("  Item:", item);
}

// 21. Try-Catch
console.log("\n21. Try-Catch");
try {
    console.log("  Try block executed");
} catch (e) {
    console.log("  Catch block");
}

// 22. Typeof Operator
console.log("\n22. Typeof Operator");
console.log("  typeof 42:", typeof 42);
console.log("  typeof 'hello':", typeof "hello");

// 23. Default Parameters
console.log("\n23. Default Parameters");
function greet(name = "World") {
    return `Hello, ${name}`;
}
console.log("  With default:", greet());
console.log("  With argument:", greet("Alice"));

// 24. Rest Parameters
console.log("\n24. Rest Parameters");
function sum(...numbers) {
    let total = 0;
    for (const n of numbers) {
        total += n;
    }
    return total;
}
console.log("  Rest params:", sum(1, 2, 3, 4, 5));

// 25. Spread Operator
console.log("\n25. Spread Operator");
const arr1 = [1, 2, 3];
const arr2 = [...arr1, 4, 5];
console.log("  Spread:", arr2);

// 26. Destructuring Arrays
console.log("\n26. Destructuring Arrays");
const [first, second] = [10, 20, 30];
console.log("  Destructured:", first, second);

// 27. Destructuring Objects
console.log("\n27. Destructuring Objects");
const { name, age } = { name: "Charlie", age: 35 };
console.log("  Destructured:", name, age);

// 28. Object Methods
console.log("\n28. Object Methods");
const calculator = {
    value: 0,
    add(n) {
        this.value += n;
        return this.value;
    }
};
console.log("  Object method:", calculator.add(5));

// 29. String Methods
console.log("\n29. String Methods");
const text = "Hello World";
console.log("  toUpperCase:", text.toUpperCase());
console.log("  toLowerCase:", text.toLowerCase());
console.log("  substring:", text.substring(0, 5));

// 30. Math Methods
console.log("\n30. Math Methods");
console.log("  Math.max:", Math.max(1, 5, 3));
console.log("  Math.min:", Math.min(1, 5, 3));
console.log("  Math.abs:", Math.abs(-5));

console.log("\n=== Test Complete ===");
