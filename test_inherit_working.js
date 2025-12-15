// Test inheritance without printing strings
class Animal {
    constructor(name) {
        this.name = name;
        this.age = 0;
    }

    getName() {
        return this.name;
    }

    setAge(years) {
        this.age = years;
    }

    getAge() {
        return this.age;
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
dog.setAge(3);

// Test methods work
const sound = dog.speak();
console.log("speak():", sound);

// Test number field works
const age = dog.getAge();
console.log("age:", age);

// Test string field exists (don't print it)
const name = dog.getName();
if (name) {
    console.log("getName() returned: something (SUCCESS)");
} else {
    console.log("getName() returned: nothing (FAIL)");
}
