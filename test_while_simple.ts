function testWhile(): number {
    let i = 0;
    while (i < 3) {
        i = i + 1;
    }
    return i;
}

function main(): number {
    return testWhile();
}