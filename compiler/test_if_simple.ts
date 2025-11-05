function max(a: number, b: number): number {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

function main(): number {
    const result = max(42, 17);
    console.log(result);
    return result;
}