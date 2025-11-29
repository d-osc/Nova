// Test async method in class

class Calculator {
    async add(a: number, b: number): number {
        return a + b;
    }
}

function main(): number {
    let calc = new Calculator();
    let result: number = calc.add(10, 32);
    if (result != 42) {
        return 1;
    }
    return 0;
}
