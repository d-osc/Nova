const obj = {
    x: 42,
    y: 100,
    getX() {
        return this.x;
    },
    getY() {
        return this.y;
    },
    getSum() {
        return this.x + this.y;
    },
    getProduct() {
        const a = this.x;
        const b = this.y;
        return a * b;
    }
};

console.log("obj.x:", obj.x);
console.log("obj.y:", obj.y);
console.log("obj.getX():", obj.getX());
console.log("obj.getY():", obj.getY());
console.log("obj.getSum():", obj.getSum());
console.log("obj.getProduct():", obj.getProduct());

console.log("\nExpected:");
console.log("x: 42");
console.log("y: 100");
console.log("getX: 42");
console.log("getY: 100");
console.log("getSum: 142");
console.log("getProduct: 4200");
