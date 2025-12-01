// Test instanceof operator
function main(): number {
    let arr = [1, 2, 3];

    // instanceof should check if object is instance of constructor
    // For now, just test compilation
    let result = arr instanceof Array;

    return result ? 1 : 0;
}
