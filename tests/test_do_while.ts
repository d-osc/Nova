// Test do-while loop
function main(): number {
    // Basic do-while: executes at least once
    let count = 0;
    do {
        count = count + 1;
    } while (count < 5);

    // Do-while that executes only once (condition false from start)
    let x = 10;
    do {
        x = x + 5;
    } while (x < 10);  // False, but body already executed once

    // Result: count(5) + x(15) = 20
    return count + x;
}
