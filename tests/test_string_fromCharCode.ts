function main(): number {
    // Create string from character code
    // 65 is ASCII for 'A'
    let str = String.fromCharCode(65);
    // Return the character code to verify it worked
    return str.charCodeAt(0);
}
