// Test constructor with string parameter
class Person {
    constructor(name) {
        console.log("constructor received:", name);
        this.name = name;
    }
}

const p = new Person("Alice");
console.log("p.name =", p.name);

console.log("\nExpected:");
console.log("constructor received: Alice");
console.log("p.name = Alice");
