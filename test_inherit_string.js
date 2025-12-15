// Test inheritance with string fields
class Animal {
    constructor(name) {
        this.name = name;
    }

    getName() {
        return this.name;
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

const dog = new Dog("Max", "Golden");
console.log("Name:", dog.getName());
console.log("Breed:", dog.getBreed());
