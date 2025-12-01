// Simple rest parameter test without type annotations
function sum(...numbers) {
    let total = 0;
    for (let i = 0; i < numbers.length; i++) {
        total = total + numbers[i];
    }
    return total;
}

function main() {
    let result1 = sum(1, 2, 3);       // Should be 6
    let result2 = sum(5, 10, 15, 20); // Should be 50
    return result1 + result2;          // Should be 56
}
