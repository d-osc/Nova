// Test template literals

const name = "John";
const age = 30;

// Simple template
const greeting = `Hello, ${name}!`;
console.log(greeting);

// Multiple expressions
const message = `My name is ${name} and I am ${age} years old.`;
console.log(message);

// Expression in template
const x = 10;
const y = 20;
const result = `${x} + ${y} = ${x + y}`;
console.log(result);

// Nested
const person = "Alice";
const activity = "coding";
const sentence = `${person} is ${activity} in JavaScript`;
console.log(sentence);
