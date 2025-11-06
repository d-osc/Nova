function manualLoop(): number {
    let i = 0;
    let result = 0;
    
    // Manual unrolling of while (i < 3)
    // Iteration 0
    if (i < 3) {
        result = result + 1;
        i = i + 1;
    }
    
    // Iteration 1
    if (i < 3) {
        result = result + 1;
        i = i + 1;
    }
    
    // Iteration 2
    if (i < 3) {
        result = result + 1;
        i = i + 1;
    }
    
    // Iteration 3 (should not execute)
    if (i < 3) {
        result = result + 1;
        i = i + 1;
    }
    
    return result;
}