// Edge case tests for classes

// Test 1: Many fields (stress test struct layout)
class BigClass {
    constructor(f1, f2, f3, f4, f5, f6, f7, f8) {
        this.f1 = f1;
        this.f2 = f2;
        this.f3 = f3;
        this.f4 = f4;
        this.f5 = f5;
        this.f6 = f6;
        this.f7 = f7;
        this.f8 = f8;
    }
}

const big = new BigClass("A", "B", "C", "D", "E", "F", "G", "H");
console.log("Field 1:", big.f1);
console.log("Field 4:", big.f4);
console.log("Field 8:", big.f8);

// Test 2: Deep inheritance chain
class L1 {
    constructor(a) { this.a = a; }
    getA() { return this.a; }
}
class L2 extends L1 {
    constructor(a, b) { super(a); this.b = b; }
    getB() { return this.b; }
}
class L3 extends L2 {
    constructor(a, b, c) { super(a, b); this.c = c; }
    getC() { return this.c; }
}
class L4 extends L3 {
    constructor(a, b, c, d) { super(a, b, c); this.d = d; }
    getD() { return this.d; }
}

const deep = new L4("First", "Second", "Third", "Fourth");
console.log("L1 field:", deep.getA());
console.log("L2 field:", deep.getB());
console.log("L3 field:", deep.getC());
console.log("L4 field:", deep.getD());

console.log("All edge cases passed!");
