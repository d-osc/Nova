interface User {
    name: string;
    age: number;
}

function greet(user: User): string {
    return \Hello, \!\;
}

const users: User[] = [
    { name: 'Alice', age: 30 },
    { name: 'Bob', age: 25 }
];

users.forEach(u => console.log(greet(u)));
