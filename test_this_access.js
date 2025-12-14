const obj = {
    x: 42,
    y: 100,
    test1() {
        return 999;  // Works!
    },
    test2() {
        const a = this.x;  // Does this work?
        return a;
    }
};

console.log("test1():", obj.test1());
console.log("test2() returning this.x:", obj.test2());
console.log("Expected:", obj.x);
