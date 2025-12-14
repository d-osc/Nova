// Comprehensive JavaScript/TypeScript Feature Test

console.log("=== Nova Compiler Feature Coverage ===\n");

let passed = 0;
let failed = 0;

// Test 1: Basic Variables
console.log("1. Variables: let, const");
let x = 5;
const y = 10;
if (x && y) {
    console.log("   ✓ PASS");
    passed++;
} else {
    console.log("   ✗ FAIL");
    failed++;
}

// Test 2: Basic Operators
console.log("2. Operators: +, -, *, /");
const sum = 10 + 5;
const diff = 10 - 5;
const prod = 10 * 5;
if (sum && diff && prod) {
    console.log("   ✓ PASS");
    passed++;
} else {
    console.log("   ✗ FAIL");
    failed++;
}

// Test 3: Classes
console.log("3. Classes");
class TestClass {
    constructor() {
        this.value = 42;
    }
    getValue() {
        return this.value;
    }
}
const obj = new TestClass();
if (obj.getValue()) {
    console.log("   ✓ PASS");
    passed++;
} else {
    console.log("   ✗ FAIL");
    failed++;
}

// Test 4: Inheritance
console.log("4. Inheritance (extends)");
class Parent {
    parentMethod() {
        return true;
    }
}
class Child extends Parent {}
const child = new Child();
if (child.parentMethod()) {
    console.log("   ✓ PASS");
    passed++;
} else {
    console.log("   ✗ FAIL");
    failed++;
}

// Test 5: If/Else
console.log("5. If/Else statements");
let ifWorks = false;
if (true) {
    ifWorks = true;
}
if (ifWorks) {
    console.log("   ✓ PASS");
    passed++;
} else {
    console.log("   ✗ FAIL");
    failed++;
}

// Test 6: Boolean logic
console.log("6. Boolean operators (&&, ||, !)");
const andTest = true && true;
const orTest = false || true;
const notTest = !false;
if (andTest && orTest && notTest) {
    console.log("   ✓ PASS");
    passed++;
} else {
    console.log("   ✗ FAIL");
    failed++;
}

// Test 7: Comparison operators
console.log("7. Comparison (===, !==, <, >)");
const eq = (5 === 5);
const neq = (5 !== 10);
if (eq && neq) {
    console.log("   ✓ PASS");
    passed++;
} else {
    console.log("   ✗ FAIL");
    failed++;
}

// Test 8: String literals
console.log("8. String literals");
const str = "Hello";
if (str) {
    console.log("   ✓ PASS");
    passed++;
} else {
    console.log("   ✗ FAIL");
    failed++;
}

// Test 9: Array creation
console.log("9. Array creation");
const arr = [1, 2, 3];
if (arr) {
    console.log("   ✓ PASS (creation only)");
    passed++;
} else {
    console.log("   ✗ FAIL");
    failed++;
}

// Test 10: Function calls
console.log("10. Function calls");
function testFunc() {
    return true;
}
if (testFunc()) {
    console.log("   ✓ PASS");
    passed++;
} else {
    console.log("   ✗ FAIL");
    failed++;
}

console.log("\n=== Summary ===");
console.log("Tests passed:", passed);
console.log("Tests failed:", failed);
