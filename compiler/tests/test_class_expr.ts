// Test class expression
function main(): number {
    let MyClass = class {
        value: number;
        constructor(v: number) {
            this.value = v;
        }
    };
    let obj = new MyClass(42);
    if (obj.value != 42) return 1;
    return 0;
}
