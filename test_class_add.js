console.log("Test: Class method with arithmetic");
class Point {
  constructor(x, y) {
    this.x = x;
    this.y = y;
  }
  
  sum() {
    return this.x + this.y;
  }
}

const p = new Point(10, 20);
const result = p.sum() + 0;  // Force it to be treated as number
console.log("Sum + 0:", result);
console.log("Done");
