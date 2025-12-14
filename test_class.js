console.log("Test: Classes");
class Point {
  constructor(x, y) {
    this.x = x;
    this.y = y;
  }
  
  distance() {
    return this.x + this.y;
  }
}

const p = new Point(3, 4);
console.log("Point x:", p.x);
console.log("Point y:", p.y);
console.log("Distance:", p.distance());
console.log("Done");
