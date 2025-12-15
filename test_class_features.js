// Test various class features
class Calculator {
    constructor(x, y) {
        this.x = x;
        this.y = y;
        this.count = 0;
    }

    add() {
        this.count = this.count + 1;
        return this.x + this.y;
    }

    multiply() {
        this.count = this.count + 1;
        return this.x * this.y;
    }

    getCount() {
        return this.count;
    }
}

const calc = new Calculator(5, 3);
console.log("Add:", calc.add());
console.log("Multiply:", calc.multiply());
console.log("Count:", calc.getCount());
