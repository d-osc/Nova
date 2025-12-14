// Comprehensive Class Test - All Features

// Test 1: Simple class with fields
class Person {
    constructor(name, age) {
        this.name = name;
        this.age = age;
    }

    getName() {
        return this.name;
    }

    getAge() {
        return this.age;
    }
}

console.log("=== Test 1: Simple Class ===");
const person = new Person("Alice", 30);
console.log("Name:", person.getName());
console.log("Age:", person.getAge());

// Test 2: Single-level inheritance
class Animal {
    constructor(name) {
        this.name = name;
    }

    getName() {
        return this.name;
    }
}

class Dog extends Animal {
    constructor(name, breed) {
        super(name);
        this.breed = breed;
    }

    getBreed() {
        return this.breed;
    }
}

console.log("=== Test 2: Single Inheritance ===");
const dog = new Dog("Max", "Golden");
console.log("Name:", dog.getName());
console.log("Breed:", dog.getBreed());

// Test 3: Multi-level inheritance
class Mammal extends Animal {
    constructor(name, legs) {
        super(name);
        this.legs = legs;
    }

    getLegs() {
        return this.legs;
    }
}

class Cat extends Mammal {
    constructor(name, legs, color) {
        super(name, legs);
        this.color = color;
    }

    getColor() {
        return this.color;
    }
}

console.log("=== Test 3: Multi-level Inheritance ===");
const cat = new Cat("Whiskers", 4, "Orange");
console.log("Name:", cat.getName());
console.log("Legs:", cat.getLegs());
console.log("Color:", cat.getColor());

// Test 4: Multiple fields
class Book {
    constructor(title, author, year, isbn) {
        this.title = title;
        this.author = author;
        this.year = year;
        this.isbn = isbn;
    }

    getTitle() {
        return this.title;
    }

    getAuthor() {
        return this.author;
    }

    getYear() {
        return this.year;
    }

    getISBN() {
        return this.isbn;
    }
}

console.log("=== Test 4: Multiple Fields ===");
const book = new Book("1984", "Orwell", 1949, "978-0451524935");
console.log("Title:", book.getTitle());
console.log("Author:", book.getAuthor());
console.log("Year:", book.getYear());
console.log("ISBN:", book.getISBN());

console.log("=== All Tests Complete ===");
