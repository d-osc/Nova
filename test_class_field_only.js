// Test that SetField actually stores the value correctly
class Test {
    constructor(value) {
        this.field = value;
    }
}

const obj = new Test(42);
console.log("Field value:", obj.field);
console.log("Expected: 42");
