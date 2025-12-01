// Test try/catch/finally
function main(): number {
    let result = 0;

    try {
        result = 10;
    } catch (e) {
        result = 20;
    } finally {
        result = result + 5;
    }

    return result;  // Should be 15 (10 + 5 from finally)
}
