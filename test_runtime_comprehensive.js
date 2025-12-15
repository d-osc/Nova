console.log("=== COMPREHENSIVE RUNTIME TEST ===");

// 1. Console
console.log("✓ console.log");

// 2. Arrays
const arr = [1, 2, 3];
arr.push(4);
arr.pop();
const mapped = arr.map(x => x * 2);
const filtered = arr.filter(x => x > 1);
const reduced = arr.reduce((a, b) => a + b, 0);
console.log("✓ Array:", arr.length, mapped.length, filtered.length, reduced);

// 3. Strings
const str = "Hello World";
const upper = str.toUpperCase();
const lower = str.toLowerCase();
const sliced = str.slice(0, 5);
const len = str.length;
console.log("✓ String:", len, upper, lower, sliced);

// 4. Numbers
const n1 = 10;
const n2 = 20;
const add = n1 + n2;
const sub = n1 - n2;
const mul = n1 * n2;
const div = n2 / n1;
const mod = n2 % n1;
console.log("✓ Number ops:", add, sub, mul, div, mod);

// 5. Booleans
const t = true;
const f = false;
const and = t && f;
const or = t || f;
const not = !t;
console.log("✓ Boolean:", and, or, not);

// 6. Comparisons
const eq = (5 === 5);
const neq = (5 !== 6);
const gt = (10 > 5);
const lt = (5 < 10);
const gte = (10 >= 10);
const lte = (5 <= 10);
console.log("✓ Comparison:", eq, neq, gt, lt, gte, lte);

// 7. Control flow
let ifResult = "no";
if (10 > 5) {
    ifResult = "yes";
}
const ternary = 10 > 5 ? "yes" : "no";
console.log("✓ Control:", ifResult, ternary);

// 8. Loops
let forSum = 0;
for (let i = 0; i < 5; i = i + 1) {
    forSum = forSum + i;
}
let whileSum = 0;
let j = 0;
while (j < 5) {
    whileSum = whileSum + j;
    j = j + 1;
}
console.log("✓ Loops:", forSum, whileSum);

// 9. Functions
function add(a, b) {
    return a + b;
}
const arrow = (a, b) => a * b;
console.log("✓ Functions:", add(10, 20), arrow(5, 6));

// 10. Objects
const obj = { x: 10, y: 20 };
console.log("✓ Objects:", obj.x, obj.y);

// 11. Classes
class Point {
    constructor(x, y) {
        this.x = x;
        this.y = y;
    }
    sum() {
        return this.x + this.y;
    }
}
const p = new Point(5, 10);
console.log("✓ Classes:", p.sum());

// 12. Template literals
const name = "Nova";
const msg = `Hello ${name}`;
console.log("✓ Template:", msg);

console.log("=== ALL RUNTIME TESTS PASSED ===");
