// Test multi-level inheritance (3 levels)

class LivingBeing {
    constructor() {
        this.alive = 1;
    }

    breathe() {
        return "breathing";
    }
}

class Animal extends LivingBeing {
    constructor() {
        this.type = "Animal";
    }

    eat() {
        return "eating";
    }
}

class Dog extends Animal {
    constructor() {
        this.breed = "Labrador";
    }

    speak() {
        return "Woof";
    }
}

const dog = new Dog();

console.log("=== Multi-Level Inheritance Test ===");
console.log("Dog instance:");
console.log("  alive:", dog.alive, "(from LivingBeing - grandparent)");
console.log("  type:", dog.type, "(from Animal - parent)");
console.log("  breed:", dog.breed, "(own field)");
console.log("");
console.log("  breathe():", dog.breathe(), "(from LivingBeing - grandparent)");
console.log("  eat():", dog.eat(), "(from Animal - parent)");
console.log("  speak():", dog.speak(), "(own method)");
console.log("");
console.log("=== Multi-Level Inheritance Working! ===");
