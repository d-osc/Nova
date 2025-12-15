// Simple super() test
class Animal {
    constructor(name) {
        this.name = name;
    }
}

class Dog extends Animal {
    constructor(name) {
        super(name);
    }
}

const dog = new Dog("Buddy");
console.log("Name:", dog.name);
