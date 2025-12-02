

function greet(user) {
    return \Hello, \!\;
}

const users = [
    { name: 'Alice', age: 30 },
    { name: 'Bob', age: 25 }
];

users.forEach(u => console.log(greet(u)));
