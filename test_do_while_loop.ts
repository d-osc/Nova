function testDoWhileLoop(): number {
    let i = 0;
    do {
        i = i + 1;
    } while (i < 5);
    return i;
}

function main(): number {
    return testDoWhileLoop();
}