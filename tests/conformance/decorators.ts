// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

// Test decorators on class methods

function logCall(target: any, key: string, desc: any) {
    // A no-op legacy descriptor decorator.
    return desc;
}

function double(target: any, key: string, desc: any) {
    const orig = desc.value;
    desc.value = function(...args: any[]) {
        return orig.apply(this, args) * 2;
    };
    return desc;
}

class Calculator {
    @logCall
    add(a: number, b: number): number {
        return a + b;
    }

    @double
    multiply(a: number, b: number): number {
        return a * b;
    }
}

function main(): number {
    const calc = new Calculator();

    // add() should work normally - decorator just records
    if (calc.add(2, 3) !== 5) return 1;

    // multiply() should be doubled by decorator
    if (calc.multiply(4, 5) !== 40) return 2;  // 4*5=20, doubled = 40

    return 0;
}
