// Test basic inheritance without super() parameters
class Animal {
    constructor() {
        this.name = "Generic";
        this.type = "Unknown";
    }

    speak() {
        return "sound";
    }
}

class Dog extends Animal {
    constructor() {
        // Don't call super() - let default handle it
        this.breed = "Labrador";
    }

    speak() {
        return "Woof";
    }
}

const dog = new Dog();

console.log("=== Basic Inheritance Test (no super) ===");
console.log("name:", dog.name, "Expected: 0 (not initialized without super)");
console.log("type:", dog.type, "Expected: 0 (not initialized without super)");
console.log("breed:", dog.breed, "Expected: Labrador");
console.log("speak():", dog.speak(), "Expected: Woof");

// Test 2: Create Animal directly
const animal = new Animal();
console.log("\n=== Animal Test ===");
console.log("name:", animal.name, "Expected: Generic");
console.log("type:", animal.type, "Expected: Unknown");
