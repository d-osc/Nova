// Comprehensive JSON.stringify test

// Test with number
const num = 42;
console.log(JSON.stringify(num));

// Test with string
const name = "Nova";
console.log(JSON.stringify(name));

// Test with boolean
const isActive = true;
console.log(JSON.stringify(isActive));

// Test with array
const numbers = [1, 2, 3, 4, 5];
console.log(JSON.stringify(numbers));

// Test that our fixed object return works with JSON
function createPerson() {
    return { age: 25, score: 100 };
}

const person = createPerson();
console.log(person.age);
console.log(person.score);
