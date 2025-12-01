// Test modulo in practical use cases
function main(): number {
    let sum = 0;

    // Use case 1: Check if numbers are even or odd
    for (let i = 0; i < 10; i++) {
        if (i % 2 === 0) {
            sum = sum + i;  // Add even numbers: 0+2+4+6+8 = 20
        }
    }

    // Use case 2: Cycle through values (modulo wrapping)
    let cycleValue = 0;
    for (let j = 0; j < 15; j++) {
        cycleValue = j % 5;  // Cycles 0,1,2,3,4,0,1,2,3,4,0,1,2,3,4
    }
    // cycleValue should be 4 at the end (14 % 5 = 4)
    sum = sum + cycleValue;  // 20 + 4 = 24

    // Use case 3: Every 3rd iteration
    for (let k = 1; k <= 9; k++) {
        if (k % 3 === 0) {
            sum = sum + 1;  // Increments at k=3,6,9 → 3 times
        }
    }

    // Final: 20 + 4 + 3 = 27
    return sum;
}
