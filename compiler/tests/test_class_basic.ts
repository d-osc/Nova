// Test basic class
class Point {
    x: number;
    y: number;

    constructor(x: number, y: number) {
        this.x = x;
        this.y = y;
    }

    sum(): number {
        return this.x + this.y;
    }
}

function main(): number {
    let p = new Point(10, 20);
    return p.sum();  // Should be 30
}
