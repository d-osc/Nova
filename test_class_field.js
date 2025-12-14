// Minimal test for class field access bug
class Person {
    constructor(name) {
        console.log("Constructor called, name =", name);
        this.name = name;
        console.log("After assignment, this.name =", this.name);
    }

    greet() {
        console.log("In greet(), this.name =", this.name);
    }
}

const p = new Person("Alice");
console.log("Direct access: p.name =", p.name);
p.greet();
