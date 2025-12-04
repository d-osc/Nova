// Function returning object test
function makePoint(x: number, y: number) {
    return { x: x, y: y };
}

const p = makePoint(100, 200);
console.log(p.x);
console.log(p.y);
