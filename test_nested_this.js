const calculator = {
    value: 10,
    add(n) {
        return this.value + n;
    },
    multiply(n) {
        return this.value * n;
    },
    compute() {
        const sum = this.add(5);
        const product = this.multiply(3);
        return sum + product;
    }
};

console.log("value:", calculator.value);
console.log("add(5):", calculator.add(5));
console.log("multiply(3):", calculator.multiply(3));
console.log("compute():", calculator.compute());

console.log("\nExpected:");
console.log("value: 10");
console.log("add(5): 15");
console.log("multiply(3): 30");
console.log("compute(): 45");
