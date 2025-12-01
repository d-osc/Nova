// Test compound assignment operators
function main(): number {
    let x = 10;
    x += 5;   // x = 15
    
    let y = 20;
    y -= 8;   // y = 12
    
    let z = 3;
    z *= 4;   // z = 12
    
    let a = 50;
    a /= 5;   // a = 10
    
    let b = 17;
    b %= 5;   // b = 2
    
    // Sum: 15 + 12 + 12 + 10 + 2 = 51
    return x + y + z + a + b;
}
