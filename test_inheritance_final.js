// Test class inheritance
class Animal {
    constructor(name) {
        this.name = name;
    }

    speak() {
        console.log("Name:", this.name);
        return 1;
    }
}

class Dog extends Animal {
    constructor(name, breed) {
        super(name);
        this.breed = breed;
    }

    getBreed() {
        return this.breed;
    }
}

const dog = new Dog("Buddy", "Golden");
dog.speak();
console.log("Breed:", dog.getBreed());
