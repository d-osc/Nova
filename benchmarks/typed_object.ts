// Typed object test with explicit return type
type Point = { x: number, y: number };

function makePoint(x: number, y: number): Point {
    return { x: x, y: y };
}

const p = makePoint(100, 200);
console.log(p.x);
