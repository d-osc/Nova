console.log("Testing default parameters:");

function greet(name, greeting = "Hello") {
    console.log(greeting, name);
}

greet("Alice", "Hi");
console.log("---");
greet("Bob");
console.log("Done");
