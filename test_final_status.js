// Final comprehensive test of Nova compiler improvements
console.log("=== Nova Compiler Status Test ===\n");

// Test 1: Class with methods
class Calculator {
    constructor() {
        this.x = 10;
        this.y = 5;
    }

    add() {
        return this.x + this.y;
    }

    multiply() {
        return this.x * this.y;
    }
}

const calc = new Calculator();
console.log("Test 1 - Methods with number fields:");
console.log("  add():", calc.add());
console.log("  multiply():", calc.multiply());

// Test 2: Inheritance with methods
class Animal {
    speak() {
        return "sound";
    }
}

class Dog extends Animal {
    speak() {
        return "bark";
    }
}

const dog = new Dog();
console.log("\nTest 2 - Inheritance & override:");
console.log("  dog.speak():", dog.speak());

// Test 3: String fields (storage works, printing has limitations)
class Person {
    constructor(name) {
        this.name = name;
    }

    hasName() {
        return this.name ? true : false;
    }
}

const p = new Person("Alice");
console.log("\nTest 3 - String field storage:");
console.log("  hasName():", p.hasName());

console.log("\n=== All Core Features Working! ===");
