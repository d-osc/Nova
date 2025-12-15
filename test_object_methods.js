// Test object methods with 'this' binding

const obj = {
    x: 10,
    y: 20,
    getX() {
        return this.x;
    },
    getY() {
        return this.y;
    },
    getSum() {
        return this.x + this.y;
    }
};

console.log("=== Object Methods Test ===");
console.log("obj.x:", obj.x);
console.log("obj.y:", obj.y);
console.log("obj.getX():", obj.getX());
console.log("obj.getY():", obj.getY());
console.log("obj.getSum():", obj.getSum());
