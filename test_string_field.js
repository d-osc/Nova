// Test string field
class Person {
    constructor(name) {
        this.name = name;
    }

    getName() {
        return this.name;
    }
}

const p = new Person("Alice");
console.log("Name:", p.getName());
