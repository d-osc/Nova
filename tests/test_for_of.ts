// Test for...of loop
function main(): number {
    let arr = [10, 20, 30];
    let sum = 0;

    // for...of should iterate over array values
    for (let value of arr) {
        sum = sum + value;
    }

    return sum;  // Should be 60 (10+20+30)
}
