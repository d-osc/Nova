// Test 05: Object Operations
// Tests object creation and manipulation

console.log('=== Test 05: Object Operations ===');

// Object creation
const person = {
    name: 'John',
    age: 30,
    city: 'New York'
};

console.log('person.name:', person.name);
console.log('person.age:', person.age);
console.log('person["city"]:', person["city"]);

// Adding properties
person.country = 'USA';
console.log('Added country:', person.country);

// Modifying properties
person.age = 31;
console.log('Modified age:', person.age);

// Nested objects
const company = {
    name: 'TechCorp',
    address: {
        street: '123 Main St',
        city: 'Boston'
    }
};

console.log('company.name:', company.name);
console.log('company.address.city:', company.address.city);

// Object with methods
const calculator = {
    value: 0,
    add: function(n) {
        calculator.value += n;
        return calculator.value;
    },
    getValue: function() {
        return calculator.value;
    }
};

calculator.add(5);
calculator.add(3);
console.log('calculator.getValue():', calculator.getValue());

// Object.keys
const keys = Object.keys(person);
console.log('Object.keys(person).length:', keys.length);

// Object.values
const values = Object.values(person);
console.log('Object.values(person).length:', values.length);

// Object.assign (Nova supports 2 arguments)
const obj1 = { a: 1, b: 2 };
const obj2 = { c: 3, d: 4 };
const temp = Object.assign({}, obj1);
const merged = Object.assign(temp, obj2);
console.log('Object.assign merged:', merged.a, merged.b, merged.c, merged.d);

console.log('\n✓ Object Operations Test Complete');
