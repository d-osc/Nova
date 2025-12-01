// Example 1: Hello World
console.log("Hello from Nova Compiler!");

// Example 2: Functions
function fibonacci(n: number): number {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

console.log("Fibonacci(10) =", fibonacci(10));

// Example 3: Async/Await
async function fetchData(): Promise<string> {
    return "Data fetched successfully";
}

async function main() {
    const result = await fetchData();
    console.log(result);
}

// Example 4: Classes
class Animal {
    constructor(public name: string) {}
    
    speak(): void {
        console.log(`${this.name} makes a sound`);
    }
}

class Dog extends Animal {
    speak(): void {
        console.log(`${this.name} barks!`);
    }
}

const dog = new Dog("Buddy");
dog.speak();

// Example 5: Array operations
const numbers = [1, 2, 3, 4, 5];
const doubled = numbers.map(n => n * 2);
console.log("Doubled:", doubled);

// Example 6: Type annotations
interface Person {
    name: string;
    age: number;
}

function greet(person: Person): string {
    return `Hello, ${person.name}! You are ${person.age} years old.`;
}

const alice: Person = { name: "Alice", age: 30 };
console.log(greet(alice));

main();
