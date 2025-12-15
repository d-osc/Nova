// Test class inheritance
console.log("Testing class inheritance:");

class Animal {
    constructor(name) {
        console.log("Animal constructor, name =", name);
        this.name = name;
    }
    speak() {
        console.log("Animal.speak()");
        return "sound";
    }
}

class Dog extends Animal {
    constructor(name) {
        console.log("Dog constructor, name =", name);
        super(name);
        console.log("After super()");
    }
    speak() {
        console.log("Dog.speak()");
        return "bark";
    }
}

const dog = new Dog("Rex");
console.log("dog.name =", dog.name);
const sound = dog.speak();
console.log("dog.speak() returned:", sound);
console.log("sound === \"bark\":", sound === "bark");
