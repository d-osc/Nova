// Test without inheritance
class Animal {
    constructor(name) {
        this.name = name;
    }

    getName() {
        return this.name;
    }
}

const animal = new Animal("Buddy");
console.log("Name:", animal.getName());
