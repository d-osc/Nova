// Test nullish coalescing operator
function main(): number {
    let x = 0;
    let y = x ?? 5;  // should be 0 (x is not null/undefined)
    if (y != 0) return 1;
    return 0;
}
