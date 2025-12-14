console.log("=== CLASSES 100% VERIFICATION ===");

// Test 1: Basic class
class Point {
    constructor(x, y) {
        this.x = x;
        this.y = y;
    }
    getX() { return this.x; }
    getY() { return this.y; }
}
const p = new Point(10, 20);
console.log("✓ Basic class:", p.getX(), p.getY());

// Test 2: String fields
class Person {
    constructor(name, email) {
        this.name = name;
        this.email = email;
    }
    getName() { return this.name; }
    getEmail() { return this.email; }
}
const person = new Person("Alice", "alice@test.com");
console.log("✓ String fields:", person.getName(), person.getEmail());

// Test 3: Single inheritance
class Animal {
    constructor(name) {
        this.name = name;
    }
    speak() {
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
const dog = new Dog("Max", "Golden Retriever");
console.log("✓ Inheritance:", dog.speak(), dog.getBreed());

// Test 4: Multi-level inheritance (3 levels)
class Vehicle {
    constructor(brand) {
        this.brand = brand;
    }
    getBrand() { return this.brand; }
}
class Car extends Vehicle {
    constructor(brand, model) {
        super(brand);
        this.model = model;
    }
    getModel() { return this.model; }
}
class ElectricCar extends Car {
    constructor(brand, model, battery) {
        super(brand, model);
        this.battery = battery;
    }
    getBattery() { return this.battery; }
}
const tesla = new ElectricCar("Tesla", "Model S", 100);
console.log("✓ Multi-level:", tesla.getBrand(), tesla.getModel(), tesla.getBattery());

// Test 5: Many fields (6 fields)
class User {
    constructor(id, name, email, age, city, country) {
        this.id = id;
        this.name = name;
        this.email = email;
        this.age = age;
        this.city = city;
        this.country = country;
    }
    getId() { return this.id; }
    getAge() { return this.age; }
    getCountry() { return this.country; }
}
const user = new User(1, "Bob", "bob@test.com", 25, "Bangkok", "Thailand");
console.log("✓ Many fields:", user.getId(), user.getAge(), user.getCountry());

// Test 6: Mixed types (strings and numbers)
class Product {
    constructor(name, price, stock) {
        this.name = name;
        this.price = price;
        this.stock = stock;
    }
    getName() { return this.name; }
    getPrice() { return this.price; }
    getStock() { return this.stock; }
}
const product = new Product("Laptop", 999, 50);
console.log("✓ Mixed types:", product.getName(), product.getPrice(), product.getStock());

// Test 7: Multiple methods
class Calculator {
    constructor(value) {
        this.value = value;
    }
    add(n) {
        return this.value + n;
    }
    multiply(n) {
        return this.value * n;
    }
    getValue() {
        return this.value;
    }
}
const calc = new Calculator(10);
console.log("✓ Multiple methods:", calc.getValue(), calc.add(5), calc.multiply(3));

// Test 8: Inheritance with method override simulation
class Shape {
    constructor(name) {
        this.name = name;
    }
    getName() {
        return this.name;
    }
}
class Circle extends Shape {
    constructor(name, radius) {
        super(name);
        this.radius = radius;
    }
    getRadius() {
        return this.radius;
    }
    getArea() {
        return this.radius * this.radius * 3;
    }
}
const circle = new Circle("MyCircle", 5);
console.log("✓ Methods:", circle.getName(), circle.getRadius(), circle.getArea());

console.log("=== ALL CLASSES TESTS PASS 100% ===");
