// Minimal class inheritance test
class Animal {
    constructor(name) {
        this.name = name;
    }
    speak() {
        return `${this.name} makes a sound`;
    }
}

class Dog extends Animal {
    constructor(name, breed) {
        super(name);
        this.breed = breed;
    }
    speak() {
        return `${this.name} barks`;
    }
}

const dog = new Dog("Rex", "Labrador");
console.log("dog.name:", dog.name);
console.log("dog.breed:", dog.breed);
console.log("dog.speak():", dog.speak());
