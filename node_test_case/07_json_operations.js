// Test 07: JSON Operations
// Tests JSON stringify and parse

console.log('=== Test 07: JSON Operations ===');

// Simple values
console.log('JSON.stringify(42):', JSON.stringify(42));
console.log('JSON.stringify("hello"):', JSON.stringify("hello"));
console.log('JSON.stringify(true):', JSON.stringify(true));
console.log('JSON.stringify(null):', JSON.stringify(null));

// Arrays
const arr = [1, 2, 3, 4, 5];
const arrJson = JSON.stringify(arr);
console.log('JSON.stringify([1,2,3,4,5]):', arrJson);

// Objects
const person = {
    name: 'John',
    age: 30,
    active: true
};
const personJson = JSON.stringify(person);
console.log('JSON.stringify(person):', personJson);

// Nested objects
const company = {
    name: 'TechCorp',
    employees: [
        { name: 'Alice', role: 'Developer' },
        { name: 'Bob', role: 'Designer' }
    ],
    founded: 2020
};
const companyJson = JSON.stringify(company);
console.log('JSON.stringify(company) length:', companyJson.length);

// Parsing
const parsed1 = JSON.parse('42');
console.log('JSON.parse("42"):', parsed1, typeof parsed1);

const parsed2 = JSON.parse('"hello"');
console.log('JSON.parse(\'"hello"\'):', parsed2, typeof parsed2);

const parsed3 = JSON.parse('[1,2,3]');
console.log('JSON.parse("[1,2,3]"):', parsed3.length, 'elements');

const parsed4 = JSON.parse('{"name":"John","age":30}');
console.log('JSON.parse(person).name:', parsed4.name);
console.log('JSON.parse(person).age:', parsed4.age);

// Round-trip
const original = { x: 10, y: 20, z: [1, 2, 3] };
const jsonString = JSON.stringify(original);
const restored = JSON.parse(jsonString);
console.log('Round-trip x:', restored.x);
console.log('Round-trip y:', restored.y);
console.log('Round-trip z.length:', restored.z.length);

console.log('\n✓ JSON Operations Test Complete');
