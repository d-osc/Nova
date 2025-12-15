// Comprehensive JavaScript Feature Coverage Test
console.log("=== JavaScript Coverage Test ===\n");

let passCount = 0;
let failCount = 0;

function test(name, fn) {
    try {
        fn();
        console.log("✓", name);
        passCount = passCount + 1;
    } catch (e) {
        console.log("✗", name);
        failCount = failCount + 1;
    }
}

// 1. Basic syntax
test("Variables (let/const)", () => {
    const x = 42;
    let y = 10;
    console.log("  x =", x, "y =", y);
});

// 2. Functions
test("Function declarations", () => {
    function add(a, b) {
        return a + b;
    }
    console.log("  add(2,3) =", add(2, 3));
});

test("Arrow functions", () => {
    const multiply = (a, b) => a * b;
    console.log("  multiply(2,3) =", multiply(2, 3));
});

// 3. Arrays
test("Array literals", () => {
    const arr = [1, 2, 3];
    console.log("  arr.length =", arr.length);
});

test("Array.push()", () => {
    const arr = [1, 2];
    arr.push(3);
    console.log("  After push:", arr.length);
});

test("Array.pop()", () => {
    const arr = [1, 2, 3];
    const val = arr.pop();
    console.log("  Popped:", val);
});

test("Array.map()", () => {
    const arr = [1, 2, 3];
    const doubled = arr.map(x => x * 2);
    console.log("  Mapped length:", doubled.length);
});

// 4. Objects
test("Object literals", () => {
    const obj = { a: 1, b: 2 };
    console.log("  obj.a =", obj.a);
});

test("Object property access", () => {
    const obj = { x: 42 };
    console.log("  obj.x =", obj.x);
});

// 5. Control flow
test("If-else", () => {
    const x = 5;
    if (x > 3) {
        console.log("  x > 3");
    } else {
        console.log("  x <= 3");
    }
});

test("For loop", () => {
    let sum = 0;
    for (let i = 0; i < 3; i++) {
        sum = sum + i;
    }
    console.log("  sum =", sum);
});

test("While loop", () => {
    let i = 0;
    while (i < 3) {
        i = i + 1;
    }
    console.log("  i =", i);
});

test("Do-while loop", () => {
    let i = 0;
    do {
        i = i + 1;
    } while (i < 3);
    console.log("  i =", i);
});

test("For-of loop", () => {
    const arr = [1, 2, 3];
    let sum = 0;
    for (const val of arr) {
        sum = sum + val;
    }
    console.log("  sum =", sum);
});

test("Switch statement", () => {
    const x = 2;
    let result = 0;
    switch (x) {
        case 1:
            result = 10;
            break;
        case 2:
            result = 20;
            break;
        default:
            result = 30;
    }
    console.log("  result =", result);
});

// 6. Operators
test("Ternary operator", () => {
    const x = 5;
    const y = x > 3 ? 10 : 20;
    console.log("  y =", y);
});

test("Logical AND (&&)", () => {
    const result = true && true;
    console.log("  result =", result);
});

test("Logical OR (||)", () => {
    const result = false || true;
    console.log("  result =", result);
});

test("typeof operator", () => {
    const x = 42;
    console.log("  typeof x =", typeof x);
});

test("Exponentiation (**)", () => {
    const result = 2 ** 3;
    console.log("  2**3 =", result);
});

test("Bitwise AND (&)", () => {
    const result = 5 & 3;
    console.log("  5&3 =", result);
});

// 7. String operations
test("String concatenation", () => {
    const s = "Hello" + " " + "World";
    console.log("  s =", s);
});

test("Template literals", () => {
    const x = 42;
    const s = `Value is ${x}`;
    console.log("  s =", s);
});

test("String.length", () => {
    const s = "Hello";
    console.log("  length =", s.length);
});

// 8. Classes
test("Class declaration", () => {
    class Animal {
        constructor(name) {
            this.name = name;
        }

        greet() {
            return this.name;
        }
    }

    const dog = new Animal("Dog");
    console.log("  name =", dog.greet());
});

test("Class inheritance", () => {
    class Animal {
        constructor(name) {
            this.name = name;
        }
    }

    class Dog extends Animal {
        constructor(name, breed) {
            super(name);
            this.breed = breed;
        }
    }

    const dog = new Dog("Buddy", "Golden");
    console.log("  name =", dog.name);
});

// 9. Advanced features
test("Closures", () => {
    function outer(x) {
        return function inner(y) {
            return x + y;
        };
    }
    const add5 = outer(5);
    console.log("  add5(3) =", add5(3));
});

test("Destructuring arrays", () => {
    const arr = [1, 2, 3];
    const [a, b] = arr;
    console.log("  a =", a, "b =", b);
});

test("Spread operator", () => {
    const arr1 = [1, 2];
    const arr2 = [...arr1, 3, 4];
    console.log("  length =", arr2.length);
});

console.log("\n=== Summary ===");
console.log("Passed:", passCount);
console.log("Failed:", failCount);
const total = passCount + failCount;
const percentage = (passCount * 100) / total;
console.log("Coverage:", percentage + "%");
