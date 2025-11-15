// Test exponentiation operator (**)
function main(): number {
    // Basic exponentiation
    let a = 2 ** 3;      // 2^3 = 8
    let b = 5 ** 2;      // 5^2 = 25
    let c = 3 ** 0;      // 3^0 = 1
    let d = 10 ** 1;     // 10^1 = 10

    // Exponentiation assignment
    let e = 2;
    e **= 4;             // 2^4 = 16

    // Result: 8 + 25 + 1 + 10 + 16 = 60
    return a + b + c + d + e;
}
