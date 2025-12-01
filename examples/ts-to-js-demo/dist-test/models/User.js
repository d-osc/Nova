"use strict";
// User model with TypeScript features

class User{
    id;
    name;
    email;
    age;
    role;

    constructor(id, name, email, role = "user") {
        this.id = id;
        this.name = name;
        this.email = email;
        this.role = role;
    }

    getInfo() {
        return `${this.name} (${this.email})`;
    }

    isAdmin() {
        return this.role === "admin";
    }

    setAge(age) {
        this.age = age;
    }
}

function createUser(id, name, email) {
    return new User(id, name, email);
}

module.exports = { createUser, User };
