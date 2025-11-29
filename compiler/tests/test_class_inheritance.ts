// Test class inheritance (extends)

class Animal {
    name: number;

    constructor(n: number) {
        this.name = n;
    }

    speak(): number {
        return this.name;
    }
}

class Dog extends Animal {
    constructor(n: number) {
        super(n);
    }

    bark(): number {
        return this.name * 2;
    }
}

function main(): number {
    let dog = new Dog(10);
    let speak = dog.speak();  // Should be 10 (inherited)
    let bark = dog.bark();    // Should be 20

    if (speak != 10) {
        return 1;
    }
    if (bark != 20) {
        return 2;
    }

    return 0;
}
