// Comprehensive typeof test
function main(): number {
    let num = 42;
    let str = "hello";
    let bool = true;
    let arr = [1, 2, 3];

    let t1 = typeof num;   // "number" = 6 chars
    let t2 = typeof str;   // "string" = 6 chars
    let t3 = typeof bool;  // "boolean" = 7 chars
    let t4 = typeof arr;   // "object" = 6 chars

    // Return sum of all lengths: 6 + 6 + 7 + 6 = 25
    return t1.length + t2.length + t3.length + t4.length;
}
