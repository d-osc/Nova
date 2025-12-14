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
console.log("Breed:", dog.getBreed());
