// Simple class test
class Person {
    name: string;
    age: number;

    constructor(name: string, age: number) {
        this.name = name;
        this.age = age;
    }

    getAge(): number {
        return this.age;
    }
}

function main(): number {
    let p = new Person("Alice", 30);
    return p.getAge();  // Should return 30
}
