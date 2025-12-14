// Test numeric inheritance
class Animal {
    constructor(age) {
        this.age = age;
    }

    getAge() {
        return this.age;
    }
}

class Dog extends Animal {
    constructor(age, size) {
        super(age);
        this.size = size;
    }

    getSize() {
        return this.size;
    }
}

const dog = new Dog(5, 30);
console.log("Age:", dog.getAge());
console.log("Size:", dog.getSize());
