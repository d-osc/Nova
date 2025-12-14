// Test basic field inheritance without super()
class Animal {
    constructor() {
        this.species = "Generic";
        this.legs = 4;
    }

    getInfo() {
        return "Animal info";
    }
}

class Dog extends Animal {
    constructor() {
        this.breed = "Unknown";
    }

    speak() {
        return "Woof";
    }
}

const dog = new Dog();

console.log("=== Field Inheritance (no super) ===");
console.log("species:", dog.species, "Expected: Generic");
console.log("legs:", dog.legs, "Expected: 4");
console.log("breed:", dog.breed, "Expected: Unknown");

console.log("\n=== Method Calls ===");
console.log("speak():", dog.speak(), "Expected: Woof");
console.log("getInfo():", dog.getInfo(), "Expected: Animal info");
