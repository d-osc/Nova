// Try to prevent inlining with more complex logic
function test(dummy: number) {
    const val = dummy > 0 ? 100 : 50;
    return { x: val, y: 200 };
}

const p = test(1);
console.log(p.x);
