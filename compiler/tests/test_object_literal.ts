// Test object literals
function main(): number {
    let obj = { x: 10, y: 20 };
    if (obj.x != 10) return 1;
    if (obj.y != 20) return 2;
    return 0;
}
