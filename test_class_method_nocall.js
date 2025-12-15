// Class with method but not calling it
class Point {
    constructor(x) {
        this.x = x;
    }

    getX() {
        return this.x;
    }
}

const p = new Point(42);
console.log("constructed");
