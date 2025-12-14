// Test 1: Basic features
const num = 42;
const str = "Hello";
const bool = true;
console.log("Test 1 - Basic:", num, str, bool);

// Test 2: Arrays and methods
const arr = [1, 2, 3, 4, 5];
const doubled = arr.map(x => x * 2);
console.log("Test 2 - Array map:", doubled);

// Test 3: Template literals
const name = "Nova";
const msg = `Compiler: ${name}`;
console.log("Test 3 - Template:", msg);

// Test 4: Classes
class Point {
    constructor(x, y) {
        this.x = x;
        this.y = y;
    }
    sum() {
        return this.x + this.y;
    }
}
const p = new Point(3, 4);
console.log("Test 4 - Class:", p.sum());

// Test 5: Try/catch
try {
    console.log("Test 5 - Try/catch: OK");
} catch (e) {
    console.log("Error:", e);
}

console.log("All tests completed!");
