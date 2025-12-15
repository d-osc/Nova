// Minimal rest parameters test
function sum(...numbers) {
    console.log("numbers:", numbers);
    console.log("numbers.length:", numbers.length);

    let total = 0;
    for (let i = 0; i < numbers.length; i++) {
        total += numbers[i];
    }
    return total;
}

console.log("sum(1, 2, 3):", sum(1, 2, 3));  // Should be 6
console.log("sum(5, 10):", sum(5, 10));       // Should be 15
