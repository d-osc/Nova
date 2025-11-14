// Test direct string literal length  
function test(): number {
    return "Hello".length;  // Should compute at compile time = 5
}

function main(): number {
    return test();
}
