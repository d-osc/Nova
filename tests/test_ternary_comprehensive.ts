// Comprehensive ternary operator tests
function main(): number {
    // Test 1: Basic ternary with runtime values
    let a = 5;
    let b = 3;
    let max = a > b ? a : b;  // Should be 5

    // Test 2: Nested ternary (find max of three)
    let x = 10;
    let y = 20;
    let z = 15;
    let maxOfThree = x > y ? (x > z ? x : z) : (y > z ? y : z);  // Should be 20

    // Test 3: Ternary with expressions
    let result = (a + b) > 7 ? (a * 2) : (b * 3);  // 8 > 7, so a*2 = 10

    // Test 4: Chained comparison
    let min = a < b ? a : b;  // Should be 3

    // Test 5: Boolean to number
    let flag = 1;
    let value = flag > 0 ? 100 : 200;  // Should be 100

    // Return sum to verify all work correctly
    // max(5) + maxOfThree(20) + result(10) + min(3) + value(100) = 138
    return max + maxOfThree + result + min + value;
}
