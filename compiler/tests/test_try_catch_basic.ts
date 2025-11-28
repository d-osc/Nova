// Basic try/catch test

function main(): number {
    let result = 0;
    
    try {
        result = 10;
        console.log(result);  // Should print: 10
    } catch (e) {
        result = -1;
        console.log(result);  // Should NOT print
    }
    
    console.log(result);  // Should print: 10
    
    // Return success code
    return 205;
}
