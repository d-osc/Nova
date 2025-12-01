// Test method overriding
class Animal {
    sound(): number {
        return 1;  // Generic animal sound
    }
}

class Dog extends Animal {
    sound(): number {
        return 2;  // Dog bark
    }
}

function main(): number {
    let animal = new Animal();
    let dog = new Dog();

    let a = animal.sound();  // Should be 1
    let d = dog.sound();     // Should be 2 (overridden)

    if (a != 1) {
        return 1;
    }
    if (d != 2) {
        return 2;
    }

    return 0;
}
