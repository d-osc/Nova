// Test new expression
class Box {
    value: number;
    constructor(v: number) {
        this.value = v;
    }
}
function main(): number {
    let b = new Box(42);
    if (b.value != 42) return 1;
    return 0;
}
