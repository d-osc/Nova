// Test method without field access
class Point {
    constructor(x, y) {
        this.x = x;
        this.y = y;
    }
    sayHello() {
        console.log("Hello from Point!");
        return 42;
    }
}

const p = new Point(10, 20);
const result = p.sayHello();
console.log("Result:", result);
