// Minimal class test
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
const result = p.sum();
console.log("Result:", result);
