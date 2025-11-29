// Test static methods and properties

class MathUtils {
    static pi: number = 314;  // Using integer for simplicity

    static double(x: number): number {
        return x * 2;
    }

    static triple(x: number): number {
        return x * 3;
    }
}

function main(): number {
    let d = MathUtils.double(5);   // Should be 10
    let t = MathUtils.triple(5);   // Should be 15

    if (d != 10) {
        return 1;
    }
    if (t != 15) {
        return 2;
    }

    return 0;
}
