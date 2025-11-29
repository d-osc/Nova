// Simpler class expression test
function main(): number {
    // Some other code before
    let a = 5 + 3;
    let b = 10 - 4;

    // Class expression
    let MyClass = class {
        val: number;
        constructor(v: number) {
            this.val = v;
        }
    };
    let instance = new MyClass(100);

    // Test
    if (instance.val != 100) return 1;

    return 0;
}
