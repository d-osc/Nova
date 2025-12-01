// Comprehensive test of compound assignment operators
function main(): number {
    // Test += with different values
    let a = 5;
    a += 10;  // a = 15
    a += a;   // a = 30
    
    // Test -= 
    let b = 100;
    b -= 25;  // b = 75
    b -= b/3; // b = 50 (75 - 25)
    
    // Test *=
    let c = 3;
    c *= 4;   // c = 12
    c *= 2;   // c = 24
    
    // Test /=
    let d = 100;
    d /= 4;   // d = 25
    d /= 5;   // d = 5
    
    // Test %=
    let e = 100;
    e %= 30;  // e = 10
    e %= 7;   // e = 3
    
    // Sum: 30 + 50 + 24 + 5 + 3 = 112
    return a + b + c + d + e;
}
