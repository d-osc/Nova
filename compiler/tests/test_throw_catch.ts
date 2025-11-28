// Throw and catch test

function main(): number {
    let result = 0;
    
    try {
        result = 10;
        throw 42;  // Throw an exception
        result = 20;  // Should NOT execute
    } catch (e) {
        result = -1;  // Should execute
        console.log(e);  // Should print: 42
    }
    
    console.log(result);  // Should print: -1
    
    // Return success code
    return 208;
}
