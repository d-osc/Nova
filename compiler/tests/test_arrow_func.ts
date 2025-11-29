// Test arrow function
function main(): number {
    let add = (a: number, b: number): number => { return a + b; };
    let result = add(3, 4);
    if (result != 7) return 1;
    return 0;
}
