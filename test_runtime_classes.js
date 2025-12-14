// Test: Class Runtime Functions
console.log("=== CLASS RUNTIME TEST ===");

class Point {
    constructor(x, y) {
        this.x = x;
        this.y = y;
    }
    sum() {
        return this.x + this.y;
    }
}

const p = new Point(10, 20);
console.log("Point.x:", p.x);
console.log("Point.y:", p.y);
console.log("Point.sum():", p.sum());

console.log("=== CLASSES WORK ===");
