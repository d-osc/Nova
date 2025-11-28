// Basic numeric enum test

enum Color {
    Red,
    Green,
    Blue
}

function main(): number {
    // Access enum values
    let r = Color.Red;
    let g = Color.Green;
    let b = Color.Blue;
    
    console.log(r);  // Should print: 0
    console.log(g);  // Should print: 1
    console.log(b);  // Should print: 2
    
    // Return success code
    return 203;
}
