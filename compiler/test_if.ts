function max(a: number, b: number): number {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

function main(): number {
    const result1 = max(10, 5);
    console.log(result1);
    
    const result2 = max(3, 8);
    console.log(result2);
    
    return result1;
}