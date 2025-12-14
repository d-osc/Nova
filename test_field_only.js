// Test just field access
class Point {
    constructor(x, y) {
        this.x = x;
        this.y = y;
    }
    getX() {
        return this.x;
    }
}
const p = new Point(10, 20);
const result = p.getX();
console.log("Result:", result);
