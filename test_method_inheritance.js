// Test method inheritance - child calling parent methods

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
    }

    speak() {
        return "Woof";
    }

    // Dog doesn't override eat() or sleep(), should inherit from Animal
}

const dog = new Dog();

console.log("=== Method Inheritance Test ===");
console.log("speak():", dog.speak(), "Expected: Woof");
console.log("eat():", dog.eat(), "Expected: eating");
console.log("sleep():", dog.sleep(), "Expected: sleeping");
