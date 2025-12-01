// Test logical NOT operator (!)
function main(): number {
    // !0 should be true (1)
    let a = !0;          // 1

    // !5 should be false (0)
    let b = !5;          // 0

    // !!5 should be true (1)
    let c = !!5;         // 1

    // !(!3) should be true (1)
    let d = !(!3);       // 1

    // Result: 1 + 0 + 1 + 1 = 3
    return a + b + c + d;
}
