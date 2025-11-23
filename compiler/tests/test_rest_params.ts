// Test rest parameters
function sum(...numbers: number[]): number {
    let total = 0;
    for (let i = 0; i < numbers.length; i++) {
        total = total + numbers[i];
    }
    return total;
}

function main(): number {
    let result1 = sum(1, 2, 3);       // Should be 6
    let result2 = sum(5, 10, 15, 20); // Should be 50
    return result1 + result2;          // Should be 56
}
