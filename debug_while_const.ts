function testWhileConst(): number {
    let i = 0;
    
    // This should execute exactly once
    if (1 < 3) {
        i = i + 1;
    }
    
    return i;
}