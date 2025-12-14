console.log("Test: Class method");
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
const result = p.sum();
console.log("Sum:", result);
console.log("Done");
