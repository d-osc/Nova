class Dog {
    constructor(name, breed) {
        this.name = name;
        this.breed = breed;
    }

    getBreed() {
        return this.breed;
    }
}

const dog = new Dog("Max", "Golden");
console.log("Direct breed:", dog.breed);
console.log("Method breed:", dog.getBreed());
