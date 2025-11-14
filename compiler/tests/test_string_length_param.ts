// Test string length with parameter (cannot optimize)
function getLength(s: string): number {
    return s.length;  // Should call strlen at runtime
}

function main(): number {
    let result = getLength("Test");
    return result;  // Should return 4
}
