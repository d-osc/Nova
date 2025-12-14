const obj = {
    x: 42
};

console.log("Direct access:", obj.x);

const obj2 = {
    x: 99,
    getX() {
        return 123;  // Return constant first to test
    }
};

console.log("obj2.x:", obj2.x);
console.log("obj2.getX():", obj2.getX());
