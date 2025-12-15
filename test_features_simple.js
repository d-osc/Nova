console.log("1. Variables");
const str = "Hello";
const num = 42;
console.log(str, num);

console.log("2. Arrays");
const arr = [1, 2, 3];
console.log(arr.length);

console.log("3. Classes");
class Person {
    constructor(name) {
        this.name = name;
    }
}
const p = new Person("Alice");
console.log(p.name);

console.log("4. Template literals");
const name = "World";
const msg = `Hello ${name}`;
console.log(msg);

console.log("Done");
