function abs(x: number): number {
    if (x < 0) {
        return -x;
    } else {
        return x;
    }
}

function sign(x: number): number {
    if (x < 0) {
        return -1;
    } else if (x > 0) {
        return 1;
    } else {
        return 0;
    }
}

function maxOfThree(a: number, b: number, c: number): number {
    if (a > b) {
        if (a > c) {
            return a;
        } else {
            return c;
        }
    } else {
        if (b > c) {
            return b;
        } else {
            return c;
        }
    }
}

function main(): number {
    const result1 = abs(-5);
    const result2 = abs(7);
    
    const sign1 = sign(-3);
    const sign2 = sign(0);
    const sign3 = sign(4);
    
    const max1 = maxOfThree(1, 2, 3);
    const max2 = maxOfThree(10, 5, 7);
    const max3 = maxOfThree(2, 8, 3);
    
    return max1 + max2 + max3;
}