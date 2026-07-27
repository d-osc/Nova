function main(): number {
    let sum = 0;
    for (let index = 0; index < 10; index++) {
        if (index == 5) continue;
        if (index == 9) break;
        sum += index;
    }
    console.log("sum=" + sum);
    return 0;
}
