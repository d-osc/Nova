// test_field_debug.js
class Test {
    constructor() {
        this.x = "Hello";  // Stored correctly to field 1
    }
}
const t = new Test();
const val = t.x;  // SEGFAULT - tries to read from ObjectHeader
console.log("val:", val);
