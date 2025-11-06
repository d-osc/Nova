function testLet(): number {
    let x = 5;
    x = x + 1; // Reassign mutable variable
    return x;
}

function main(): number {
    return testLet();
}