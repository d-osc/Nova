// Object Benchmark - Object literal with typed fields
interface Person {
    name: string;
    age: number;
    active: boolean;
}

function createPerson(name: string, age: number): Person {
    return {
        name: name,
        age: age,
        active: true
    };
}

function getAge(p: Person): number {
    return p.age;
}

const person = createPerson("Nova", 1);
console.log(person.age);
console.log(getAge(person));
