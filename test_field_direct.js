// Direct field access without method
class Point {
    constructor(x, y) {
        this.x = x;
        this.y = y;
    }
}

const p = new Point(10, 20);
console.log("Point created");
// Note: Direct field access like p.x is not supported yet in Nova
// This is a limitation we need to document
