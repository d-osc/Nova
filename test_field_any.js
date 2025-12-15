// Test field storage with Any type parameter
class Box {
    constructor(value) {
        console.log("storing value...");
        this.data = value;
    }

    getData() {
        return this.data;
    }
}

const b1 = new Box(42);
console.log("b1.getData():", b1.getData());

const b2 = new Box("hello");
console.log("b2.getData():", b2.getData());

console.log("\nExpected:");
console.log("b1.getData(): 42");
console.log("b2.getData(): hello");
