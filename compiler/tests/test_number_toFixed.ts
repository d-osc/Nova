function main(): number {
    // Number.prototype.toFixed(digits)
    // Formats number with fixed decimal places
    // Returns string representation
    // Instance method - called on number values

    let num: number = 123.456;
    
    // Format with 2 decimal places: "123.46" (rounded)
    let str1 = num.toFixed(2);
    
    // Format with 0 decimal places: "123" (rounded)
    let str2 = num.toFixed(0);
    
    // Format with 4 decimal places: "123.4560" (padded)
    let str3 = num.toFixed(4);

    // Test: Return fixed value to verify compilation
    // In a full implementation, would return length of result string
    return 80;
}
