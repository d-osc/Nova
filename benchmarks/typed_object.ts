// Typed object test
// Note: type aliases removed for Nova compatibility

function makePoint(x: number, y: number) {
    return { x: x, y: y };
}

const p = makePoint(100, 200);
console.log(p.x);
