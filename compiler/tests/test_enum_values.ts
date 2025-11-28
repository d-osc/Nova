// Enum with explicit values test

enum Status {
    Pending = 1,
    Active = 5,
    Completed = 10
}

function main(): number {
    // Access enum values with explicit initializers
    let p = Status.Pending;
    let a = Status.Active;
    let c = Status.Completed;
    
    console.log(p);  // Should print: 1
    console.log(a);  // Should print: 5
    console.log(c);  // Should print: 10
    
    // Return success code
    return 204;
}
