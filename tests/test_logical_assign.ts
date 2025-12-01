// Test logical assignment operators
function main(): number {
    // Test &&= (AND assignment)
    let a = 5;
    a &&= 10;  // a is truthy, so a = 10
    
    let b = 0;
    b &&= 10;  // b is falsy, so b stays 0
    
    // Test ||= (OR assignment)
    let c = 0;
    c ||= 20;  // c is falsy, so c = 20
    
    let d = 5;
    d ||= 20;  // d is truthy, so d stays 5
    
    // Test ??= (Nullish coalescing assignment)
    let e = 0;
    e ??= 30;  // 0 is not null/undefined, so e stays 0
    
    // Result: a(10) + b(0) + c(20) + d(5) + e(0) = 35
    return a + b + c + d + e;
}
