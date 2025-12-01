// Test potentially missing operators and expressions

function main(): number {
    let errors = 0;

    // 1. Logical AND assignment (&&=)
    let a1 = true;
    // a1 &&= false;  // Test separately

    // 2. Logical OR assignment (||=)
    let a2 = false;
    // a2 ||= true;  // Test separately

    // 3. Nullish coalescing (??)
    // let n1 = null ?? 5;  // Test separately

    // 4. Nullish coalescing assignment (??=)
    // let n2 = null;
    // n2 ??= 10;  // Test separately

    // 5. Strict equality/inequality
    if ((5 === 5) != true) errors = errors + 1;
    if ((5 !== 3) != true) errors = errors + 1;

    // 6. instanceof - needs runtime
    // 7. in - needs runtime
    // 8. delete - needs runtime

    return errors;
}
