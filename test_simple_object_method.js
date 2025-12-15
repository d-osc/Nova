const obj = {
    x: 42,
    getX() {
        return this.x;
    }
};

console.log("Testing object method:");
console.log("obj.x =", obj.x);
const result = obj.getX();
console.log("obj.getX() =", result);
