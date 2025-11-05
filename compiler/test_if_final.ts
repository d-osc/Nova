function isEven(n: number): number {
    if (n % 2 === 0) {
        return 1;  // true as 1
    } else {
        return 0;  // false as 0
    }
}

function main(): number {
    const even1 = isEven(4);
    const even2 = isEven(7);
    
    // Just return a value based on the results
    if (even1 === 1) {
        return 42;
    } else {
        return 0;
    }
}