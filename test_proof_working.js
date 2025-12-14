// Prove that core functionality works without console.log
class Calculator {
    constructor() {
        this.x = 10;
        this.y = 5;
        this.result = 0;
    }

    add() {
        return this.x + this.y;
    }

    storeResult(value) {
        this.result = value;
    }

    checkResult(expected) {
        return this.result === expected;
    }
}

const calc = new Calculator();
const sum = calc.add();  // Should return 15

calc.storeResult(sum);  // Store the result

// Test if the result matches expected value
if (calc.checkResult(15)) {
    console.log("SUCCESS: All operations work correctly!");
    console.log("- Methods can compute values");
    console.log("- Fields can store/retrieve");
    console.log("- Comparisons work");
} else {
    console.log("FAIL: Something wrong");
}

// Test inheritance
class Animal {
    isAnimal() {
        return true;
    }
}

class Dog extends Animal {
    isDog() {
        return true;
    }
}

const dog = new Dog();
if (dog.isAnimal() && dog.isDog()) {
    console.log("\nSUCCESS: Inheritance works!");
    console.log("- Child has parent methods");
    console.log("- Child has own methods");
}
