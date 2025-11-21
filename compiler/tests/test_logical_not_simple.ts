// Simple test for logical NOT
function main(): number {
    let a = !0;      // Should be 1 (true)
    let b = !1;      // Should be 0 (false)

    // Convert to number explicitly
    let c = a ? 1 : 0;  // 1
    let d = b ? 1 : 0;  // 0

    return c + d;  // Should be 1
}
