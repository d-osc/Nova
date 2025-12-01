// Loop tests - Nova test format

function main(): number {
    let failures = 0;

    // For loop sum
    let sum = 0;
    for (let i = 1; i <= 5; i++) {
        sum = sum + i;
    }
    if (sum !== 15) failures = failures + 1;

    // While loop count
    let count = 0;
    let j = 0;
    while (j < 10) {
        count = count + 1;
        j = j + 1;
    }
    if (count !== 10) failures = failures + 1;

    // Nested loops
    let total = 0;
    for (let a = 0; a < 3; a++) {
        for (let b = 0; b < 3; b++) {
            total = total + 1;
        }
    }
    if (total !== 9) failures = failures + 1;

    return failures;
}
