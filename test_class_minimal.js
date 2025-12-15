// Minimal class test
class Test {
    constructor() {
        this.value = 42;
    }
    
    getValue() {
        return this.value;
    }
}

const t = new Test();
console.log("calling getValue...");
const result = t.getValue();
console.log("result:", result);
