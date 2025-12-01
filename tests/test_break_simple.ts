function main(): number {
    let count = 0;
    for (let i = 0; i < 10; i = i + 1) {
        if (i == 3) {
            break;
        }
        count = count + 1;
    }
    return count;
}
