// Test simple class without inheritance first
class Animal {
    constructor(name) {
        this.name = name;
    }
    speak() {
        return "sound";
    }
}

const animal = new Animal("Fluffy");
console.log("Animal name:", animal.name, "Expected: Fluffy");
console.log("Animal speak:", animal.speak(), "Expected: sound");
