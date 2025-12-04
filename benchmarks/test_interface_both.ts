// Interface with both fields
interface Point {
    x: number;
    y: number;
}

function makePoint(px: number, py: number): Point {
    return { x: px, y: py };
}

const p = makePoint(100, 200);
console.log(p.x);
console.log(p.y);
