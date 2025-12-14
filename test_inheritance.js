// Test class inheritance

console.log("=== Test 1: Basic inheritance ===");
class Animal {
    constructor(name) {
        this.name = name;
    }
    speak() {
        return "sound";
    }
}

class Dog extends Animal {
    constructor(name) {
        super(name);
    }
    speak() {
        return "bark";
    }
}

const dog = new Dog("Rex");
console.log("Dog name:", dog.name, "Expected: Rex");
console.log("Dog speak:", dog.speak(), "Expected: bark");

console.log("=== Test 2: Method override ===");
class Cat extends Animal {
    speak() {
        return "meow";
    }
}

const cat = new Cat("Whiskers");
console.log("Cat speak:", cat.speak(), "Expected: meow");

console.log("=== Test 3: Parent method access ===");
class Bird extends Animal {
    speak() {
        const parentSound = super.speak();
        return parentSound + " chirp";
    }
}

const bird = new Bird("Tweety");
console.log("Bird speak:", bird.speak(), "Expected: sound chirp");
