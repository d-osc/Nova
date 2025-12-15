console.log("=== JavaScript Feature Check ===");
let passed = 0;
let total = 0;

// Test 1: Variables
try {
    total++;
    const x = 42;
    let y = 100;
    if (x === 42 && y === 100) {
        console.log("✓ Variables");
        passed++;
    }
} catch (e) { console.log("✗ Variables"); }

// Test 2: Arrays
try {
    total++;
    const arr = [1, 2, 3];
    if (arr[0] === 1) {
        console.log("✓ Arrays");
        passed++;
    }
} catch (e) { console.log("✗ Arrays"); }

// Test 3: Spread
try {
    total++;
    const a1 = [1, 2];
    const a2 = [...a1, 3];
    if (a2[0] === 1 && a2[2] === 3) {
        console.log("✓ Spread");
        passed++;
    }
} catch (e) { console.log("✗ Spread"); }

// Test 4: Objects
try {
    total++;
    const obj = { x: 10, y: 20 };
    if (obj.x === 10) {
        console.log("✓ Objects");
        passed++;
    }
} catch (e) { console.log("✗ Objects"); }

// Test 5: Arrow functions
try {
    total++;
    const add = (a, b) => a + b;
    if (add(2, 3) === 5) {
        console.log("✓ Arrow functions");
        passed++;
    }
} catch (e) { console.log("✗ Arrow functions"); }

// Test 6: Classes
try {
    total++;
    class Point {
        constructor(x) {
            this.x = x;
        }
    }
    const p = new Point(5);
    if (p.x === 5) {
        console.log("✓ Classes");
        passed++;
    }
} catch (e) { console.log("✗ Classes"); }

// Test 7: For loops
try {
    total++;
    let sum = 0;
    for (let i = 0; i < 3; i++) {
        sum = sum + i;
    }
    if (sum === 3) {
        console.log("✓ For loops");
        passed++;
    }
} catch (e) { console.log("✗ For loops"); }

// Test 8: Template literals
try {
    total++;
    const name = "World";
    const msg = `Hello ${name}`;
    console.log("✓ Template literals");
    passed++;
} catch (e) { console.log("✗ Template literals"); }

console.log("");
console.log("TOTAL:", passed, "/", total);
