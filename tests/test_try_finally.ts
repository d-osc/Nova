// Try/finally test

function main(): number {
    let result = 0;
    
    try {
        result = 10;
        console.log(result);  // Should print: 10
    } finally {
        result = 20;
        console.log(result);  // Should print: 20 (always runs)
    }
    
    console.log(result);  // Should print: 20
    
    // Return success code
    return 206;
}
