console.log("Testing class method call:");

class Point {
  constructor(x, y) {
    this.x = x;
    this.y = y;
  }
  sum() {
    console.log("Inside sum method");
    console.log("this.x =", this.x);
    console.log("this.y =", this.y);
    return this.x + this.y;
  }
}

const p = new Point(3, 4);
console.log("Created point, x =", p.x);
console.log("Created point, y =", p.y);
console.log("About to call sum()...");
const result = p.sum();
console.log("Sum result:", result);
console.log("Done!");
