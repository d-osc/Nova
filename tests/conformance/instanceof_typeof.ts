// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

// Test instanceof and typeof operators

class Animal {
    name: string;
    constructor(n: string) {
        this.name = n;
    }
}

class Dog extends Animal {
    bark(): string { return "woof"; }
}

class Cat extends Animal {
    meow(): string { return "meow"; }
}

function main(): number {
    // typeof primitives
    if (typeof "abc" !== "string") return 1;
    if (typeof 42 !== "number") return 2;
    if (typeof true !== "boolean") return 3;
    if (typeof undefined !== "undefined") return 4;
    if (typeof null !== "object") return 5;  // JavaScript quirk
    if (typeof Symbol() !== "symbol") return 6;
    if (typeof (() => 1) !== "function") return 7;
    if (typeof function f() { return 0; } !== "function") return 8;
    if (typeof {} !== "object") return 9;
    if (typeof [] !== "object") return 10;

    // typeof class instances
    const a: Animal = new Animal("generic");
    const d: Dog = new Dog("rex");
    const c: Cat = new Cat("tom");

    if (typeof a !== "object") return 11;
    if (typeof d !== "object") return 12;

    // instanceof basics
    if (!(a instanceof Animal)) return 13;
    if (!(d instanceof Animal)) return 14;  // Dog extends Animal
    if (!(d instanceof Dog)) return 15;
    if (!(c instanceof Animal)) return 16;
    if (!(c instanceof Cat)) return 17;

    // Negative cases
    if (a instanceof Dog) return 18;  // Animal is not Dog
    if (d instanceof Cat) return 19;  // Dog is not Cat

    // Arrays
    const arr = [1, 2, 3];
    if (!(arr instanceof Array)) return 20;
    if (!(arr instanceof Object)) return 21;

    // Functions
    function fn(): void {}
    if (!(fn instanceof Function)) return 22;
    if (!(fn instanceof Object)) return 23;

    // Objects
    const o = {};
    if (!(o instanceof Object)) return 24;
    if (o instanceof Array) return 25;

    // Errors
    const err: any = new Error("x");
    if (!(err instanceof Error)) return 26;
    if (!(err instanceof Object)) return 27;

    // string vs number
    if (typeof (1 + 1) !== "number") return 28;
    if (typeof ("a" + "b") !== "string") return 29;

    return 0;
}
