// Try/catch/finally test

function main(): number {
    let result = 0;
    
    try {
        result = 10;
        console.log(result);  // Should print: 10
    } catch (e) {
        result = -1;
        console.log(result);  // Should NOT print (no exception)
    } finally {
        result = result + 5;
        console.log(result);  // Should print: 15 (10 + 5)
    }
    
    console.log(result);  // Should print: 15
    
    // Return success code
    return 207;
}
