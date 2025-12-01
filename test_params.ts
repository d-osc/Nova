// Test case: Parameter variations
function oneParam(a: number): number {
    return a * 2;
}

function twoParams(a: number, b: number): number {
    return a + b;
}

function threeParams(a: number, b: number, c: number): number {
    return a + b + c;
}

function fourParams(a: number, b: number, c: number, d: number): number {
    return a + b + c + d;
}

function fiveParams(a: number, b: number, c: number, d: number, e: number): number {
    return a + b + c + d + e;
}

function testAll(): number {
    return oneParam(1) + 
           twoParams(2, 3) + 
           threeParams(4, 5, 6) + 
           fourParams(7, 8, 9, 10) +
           fiveParams(11, 12, 13, 14, 15);
}
