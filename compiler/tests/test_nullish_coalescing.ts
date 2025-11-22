// Test nullish coalescing operator (??)
function main(): number {
    let a = 0;
    let b = 42;
    
    // 0 is falsy but not null/undefined, so should return 0
    let result1 = a ?? 10;  // Should be 0
    
    // Regular value, should return it  
    let result2 = b ?? 10;  // Should be 42
    
    return result1 + result2;  // Should be 42 (0 + 42)
}
