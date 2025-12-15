// Comprehensive class test - 100% functionality
class Point {
    constructor(x, y) {
        this.x = x;
        this.y = y;
    }

    getX() {
        return this.x;
    }

    getY() {
        return this.y;
    }

    sum() {
        return this.x + this.y;
    }

    setX(newX) {
        this.x = newX;
    }
}

const p = new Point(10, 20);
console.log("X:", p.getX());
console.log("Y:", p.getY());
console.log("Sum:", p.sum());

p.setX(15);
console.log("New X:", p.getX());
console.log("New Sum:", p.sum());
