// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

// Test super() constructor calls and super.method() calls

class Vehicle {
    wheels: number;
    speed: number;

    constructor(wheels: number) {
        this.wheels = wheels;
        this.speed = 0;
    }

    describe(): string {
        return `vehicle with ${this.wheels} wheels at ${this.speed}`;
    }

    accelerate(): number {
        this.speed = this.speed + 10;
        return this.speed;
    }
}

class Car extends Vehicle {
    brand: string;

    constructor(brand: string) {
        super(4);  // super() constructor call
        this.brand = brand;
    }

    describe(): string {
        return `${this.brand}: ${super.describe()}`;  // super.method()
    }

    accelerateFast(): number {
        super.accelerate();
        super.accelerate();
        return super.accelerate();  // 3 calls total
    }
}

class SportsCar extends Car {
    topSpeed: number;

    constructor(brand: string, topSpeed: number) {
        super(brand);  // chain through multiple levels
        this.topSpeed = topSpeed;
    }

    describe(): string {
        return `${super.describe()} (top ${this.topSpeed})`;
    }
}

class Base {
    greet(): string { return "base"; }
}

class Child extends Base {
    greet(): string {
        const parentGreet = super.greet();
        return `child of ${parentGreet}`;
    }
}

class Grandchild extends Child {
    greet(): string {
        return `grand-${super.greet()}`;
    }
}

class Counter {
    count: number = 0;

    inc(): number {
        this.count = this.count + 1;
        return this.count;
    }
}

class DoubleCounter extends Counter {
    incTwice(): number {
        super.inc();
        return super.inc();
    }
}

function main(): number {
    // Basic super() — constructor chain
    const car = new Car("Toyota");
    if (car.wheels !== 4) return 1;
    if (car.brand !== "Toyota") return 2;
    if (car.speed !== 0) return 3;

    // super.method() — child calls parent
    const desc = car.describe();
    if (desc !== "Toyota: vehicle with 4 wheels at 0") return 4;

    // super.method() — chain of calls
    const finalSpeed = car.accelerateFast();
    if (finalSpeed !== 30) return 5;  // 10*3

    // Multi-level inheritance with super
    const sc = new SportsCar("Ferrari", 350);
    if (sc.wheels !== 4) return 6;
    if (sc.brand !== "Ferrari") return 7;
    if (sc.topSpeed !== 350) return 8;

    const scDesc = sc.describe();
    // SportsCar.describe -> Car.describe -> Vehicle.describe
    if (scDesc !== "Ferrari: vehicle with 4 wheels at 0 (top 350)") return 9;

    // Multi-level super chain
    const gc = new Grandchild();
    if (gc.greet() !== "grand-child of base") return 10;

    // Calling super.method() multiple times
    const dc = new DoubleCounter();
    if (dc.incTwice() !== 2) return 11;
    if (dc.count !== 2) return 12;

    return 0;
}
