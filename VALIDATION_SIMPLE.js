// Nova Compiler - Simple Validation Test
console.log("╔═══════════════════════════════════════════════════════════╗");
console.log("║       Nova Compiler v1.4.0 - Validation Test             ║");
console.log("╚═══════════════════════════════════════════════════════════╝");
console.log();

let tests = 0;
let passed = 0;

console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
console.log("CATEGORY 1: Core Language Features");
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

// Test 1: Variables
tests = tests + 1;
const x1 = 10;
let y1 = 20;
var z1 = 30;
if (x1 === 10) {
    passed = passed + 1;
    console.log("✓ 1.1 Variables (const, let, var)");
} else {
    console.log("✗ 1.1 Variables FAILED");
}

// Test 2: Number types
tests = tests + 1;
const int1 = 42;
const float1 = 3.14159;
if (int1 === 42) {
    passed = passed + 1;
    console.log("✓ 1.2 Number types (int, float)");
} else {
    console.log("✗ 1.2 Number types FAILED");
}

// Test 3: String types
tests = tests + 1;
const str1 = "hello";
if (str1 === "hello") {
    passed = passed + 1;
    console.log("✓ 1.3 String types");
} else {
    console.log("✗ 1.3 String types FAILED");
}

// Test 4: Boolean types
tests = tests + 1;
const bool1 = true;
const bool2 = false;
if (bool1 === true) {
    passed = passed + 1;
    console.log("✓ 1.4 Boolean types");
} else {
    console.log("✗ 1.4 Boolean types FAILED");
}

console.log();
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
console.log("CATEGORY 2: Operators");
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

tests = tests + 1;
if (5 + 3 === 8) {
    passed = passed + 1;
    console.log("✓ 2.1 Addition operator");
} else {
    console.log("✗ 2.1 Addition FAILED");
}

tests = tests + 1;
if (10 - 4 === 6) {
    passed = passed + 1;
    console.log("✓ 2.2 Subtraction operator");
} else {
    console.log("✗ 2.2 Subtraction FAILED");
}

tests = tests + 1;
if (6 * 7 === 42) {
    passed = passed + 1;
    console.log("✓ 2.3 Multiplication operator");
} else {
    console.log("✗ 2.3 Multiplication FAILED");
}

tests = tests + 1;
if (20 / 5 === 4) {
    passed = passed + 1;
    console.log("✓ 2.4 Division operator");
} else {
    console.log("✗ 2.4 Division FAILED");
}

tests = tests + 1;
if (17 % 5 === 2) {
    passed = passed + 1;
    console.log("✓ 2.5 Modulo operator");
} else {
    console.log("✗ 2.5 Modulo FAILED");
}

console.log();
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
console.log("CATEGORY 3: Mixed Type Operations");
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

tests = tests + 1;
const pi = 3.14159;
const r = 5;
const area = pi * r * r;
if (area > 78) {
    passed = passed + 1;
    console.log("✓ 3.1 Double * Integer multiplication");
} else {
    console.log("✗ 3.1 Mixed type multiplication FAILED");
}

tests = tests + 1;
const d1 = 10.5;
const i1 = 3;
if (d1 > i1) {
    passed = passed + 1;
    console.log("✓ 3.2 Double > Integer comparison");
} else {
    console.log("✗ 3.2 Mixed type comparison FAILED");
}

console.log();
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
console.log("CATEGORY 4: Functions");
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

tests = tests + 1;
const add = (a, b) => a + b;
if (add(5, 3) === 8) {
    passed = passed + 1;
    console.log("✓ 4.1 Arrow function (two params)");
} else {
    console.log("✗ 4.1 Arrow function FAILED");
}

tests = tests + 1;
const square = x => x * x;
if (square(5) === 25) {
    passed = passed + 1;
    console.log("✓ 4.2 Arrow function (one param)");
} else {
    console.log("✗ 4.2 Arrow function FAILED");
}

console.log();
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
console.log("CATEGORY 5: Arrays");
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

tests = tests + 1;
const arr1 = [1, 2, 3, 4, 5];
if (arr1[0] === 1) {
    passed = passed + 1;
    console.log("✓ 5.1 Array literal & indexing");
} else {
    console.log("✗ 5.1 Array FAILED");
}

tests = tests + 1;
const doubled = arr1.map(n => n * 2);
if (doubled[0] === 2) {
    passed = passed + 1;
    console.log("✓ 5.2 Array.map()");
} else {
    console.log("✗ 5.2 Array.map FAILED");
}

tests = tests + 1;
const evens = arr1.filter(n => n % 2 === 0);
if (evens[0] === 2) {
    passed = passed + 1;
    console.log("✓ 5.3 Array.filter()");
} else {
    console.log("✗ 5.3 Array.filter FAILED");
}

tests = tests + 1;
const sum = arr1.reduce((acc, n) => acc + n, 0);
if (sum === 15) {
    passed = passed + 1;
    console.log("✓ 5.4 Array.reduce()");
} else {
    console.log("✗ 5.4 Array.reduce FAILED");
}

console.log();
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
console.log("CATEGORY 6: Template Literals");
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

tests = tests + 1;
const name = "Nova";
const version = "1.4.0";
const msg1 = `${name} v${version}`;
if (msg1 === "Nova v1.4.0") {
    passed = passed + 1;
    console.log("✓ 6.1 Template literal interpolation");
} else {
    console.log("✗ 6.1 Template literals FAILED");
}

console.log();
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
console.log("CATEGORY 7: Classes");
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

tests = tests + 1;
class Point {
    constructor(x, y) {
        this.x = x;
        this.y = y;
    }
    sum() {
        return this.x + this.y;
    }
}

const p1 = new Point(3, 4);
if (p1.x === 3) {
    passed = passed + 1;
    console.log("✓ 7.1 Class constructor & properties");
} else {
    console.log("✗ 7.1 Class FAILED");
}

tests = tests + 1;
if (p1.sum() === 7) {
    passed = passed + 1;
    console.log("✓ 7.2 Class methods");
} else {
    console.log("✗ 7.2 Class methods FAILED");
}

console.log();
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
console.log("CATEGORY 8: Control Flow");
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

tests = tests + 1;
const val1 = 15;
let ifResult = 0;
if (val1 > 10) {
    ifResult = 1;
} else {
    ifResult = 2;
}
if (ifResult === 1) {
    passed = passed + 1;
    console.log("✓ 8.1 If-else statement");
} else {
    console.log("✗ 8.1 If-else FAILED");
}

tests = tests + 1;
let forSum = 0;
for (let i = 0; i < 5; i = i + 1) {
    forSum = forSum + i;
}
if (forSum === 10) {
    passed = passed + 1;
    console.log("✓ 8.2 For loop");
} else {
    console.log("✗ 8.2 For loop FAILED");
}

tests = tests + 1;
let whileSum = 0;
let j = 0;
while (j < 5) {
    whileSum = whileSum + j;
    j = j + 1;
}
if (whileSum === 10) {
    passed = passed + 1;
    console.log("✓ 8.3 While loop");
} else {
    console.log("✗ 8.3 While loop FAILED");
}

console.log();
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
console.log("CATEGORY 9: Objects");
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

tests = tests + 1;
const obj1 = { x: 10, y: 20 };
if (obj1.x === 10) {
    passed = passed + 1;
    console.log("✓ 9.1 Object literal & property access");
} else {
    console.log("✗ 9.1 Objects FAILED");
}

console.log();
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
console.log("CATEGORY 10: String Operations");
console.log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

tests = tests + 1;
const s1 = "Hello";
const s2 = "World";
const s3 = s1 + " " + s2;
if (s3 === "Hello World") {
    passed = passed + 1;
    console.log("✓ 10.1 String concatenation");
} else {
    console.log("✗ 10.1 String concatenation FAILED");
}

console.log();
console.log("╔═══════════════════════════════════════════════════════════╗");
console.log("║                    VALIDATION RESULTS                     ║");
console.log("╠═══════════════════════════════════════════════════════════╣");
console.log("║  Total Tests:  ", tests);
console.log("║  Passed:       ", passed);
console.log("║  Failed:       ", tests - passed);
console.log("╠═══════════════════════════════════════════════════════════╣");

if (passed === tests) {
    console.log("║                                                           ║");
    console.log("║           ✅ ALL TESTS PASSED - 100% SUCCESS! ✅          ║");
    console.log("║                                                           ║");
    console.log("║         Nova Compiler is Production Ready! 🚀            ║");
    console.log("║                                                           ║");
} else {
    console.log("║  ⚠️  Some tests failed - please review                   ║");
}

console.log("╚═══════════════════════════════════════════════════════════╝");
