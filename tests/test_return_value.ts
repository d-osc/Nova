function getValue(): number {
    return 42;
}

function main(): number {
    let result: number = getValue();
    if (result !== 42) {
        return 100;
    }
    return result;
}
