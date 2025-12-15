// Test classes only
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
console.log("Sum:", p.sum());
