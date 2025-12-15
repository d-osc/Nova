// Comprehensive inheritance test
class Animal {
    constructor(name, age) {
        this.name = name;
        this.age = age;
    }

    getInfo() {
        return this.age;
    }
}

class Dog extends Animal {
    constructor(name, age, breed) {
        super(name, age);
        this.breed = breed;
    }

    getBreed() {
        return this.breed;
    }

    getAllInfo() {
        const age = this.getInfo();
        const breed = this.getBreed();
        return age + breed;
    }
}

const dog = new Dog(100, 5, 30);
console.log("Age:", dog.getInfo());
console.log("Breed:", dog.getBreed());
console.log("All:", dog.getAllInfo());
