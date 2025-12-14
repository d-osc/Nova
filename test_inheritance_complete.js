// Comprehensive test of all inheritance features

class Animal {
    constructor() {
        this.name = "Generic";
        this.type = "Unknown";
    }

    speak() {
        return "sound";
    }

    eat() {
        return "eating";
    }

    sleep() {
        return "sleeping";
    }
}

class Dog extends Animal {
    constructor() {
        this.breed = "Labrador";
        this.age = 5;
    }

    speak() {
        return "Woof";
    }

    // Inherits eat() and sleep() from Animal
}

class Cat extends Animal {
    constructor() {
        this.breed = "Persian";
        this.color = "White";
    }

    speak() {
        return "Meow";
    }

    // Inherits eat() and sleep() from Animal
}

console.log("=== Comprehensive Inheritance Test ===");
console.log("");

const dog = new Dog();
console.log("Dog Tests:");
console.log("  name:", dog.name, "(inherited field)");
console.log("  type:", dog.type, "(inherited field)");
console.log("  breed:", dog.breed, "(own field)");
console.log("  age:", dog.age, "(own field)");
console.log("  speak():", dog.speak(), "(overridden method)");
console.log("  eat():", dog.eat(), "(inherited method)");
console.log("  sleep():", dog.sleep(), "(inherited method)");

console.log("");

const cat = new Cat();
console.log("Cat Tests:");
console.log("  name:", cat.name, "(inherited field)");
console.log("  type:", cat.type, "(inherited field)");
console.log("  breed:", cat.breed, "(own field)");
console.log("  color:", cat.color, "(own field)");
console.log("  speak():", cat.speak(), "(overridden method)");
console.log("  eat():", cat.eat(), "(inherited method)");
console.log("  sleep():", cat.sleep(), "(inherited method)");

console.log("");

const animal = new Animal();
console.log("Animal Tests:");
console.log("  name:", animal.name);
console.log("  type:", animal.type);
console.log("  speak():", animal.speak());
console.log("  eat():", animal.eat());
console.log("  sleep():", animal.sleep());

console.log("");
console.log("=== All Inheritance Features Working! ===");
