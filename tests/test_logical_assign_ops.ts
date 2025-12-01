// Test logical assignment operators
function main(): number {
    let a = 5;
    let b = 0;
    let c = 10;

    // &&= : assigns if left is truthy
    a &&= 20;  // a is truthy (5), so a = 20

    // ||= : assigns if left is falsy
    b ||= 15;  // b is falsy (0), so b = 15

    // ??= : assigns if left is null/undefined (for now, never assigns since no null/undefined)
    c ??= 100;  // c is not null/undefined (10), so c stays 10

    return a + b + c;  // Should be 20 + 15 + 10 = 45
}
