// Comprehensive class test with multiple instances and methods
class Rectangle {
    width: number;
    height: number;

    constructor(w: number, h: number) {
        this.width = w;
        this.height = h;
    }

    area(): number {
        return this.width * this.height;
    }

    perimeter(): number {
        return 2 * (this.width + this.height);
    }
}

function main(): number {
    let rect1 = new Rectangle(5, 3);
    let rect2 = new Rectangle(10, 4);

    let area1 = rect1.area();      // Should be 15
    let area2 = rect2.area();      // Should be 40
    let perim1 = rect1.perimeter(); // Should be 16

    // Return sum to verify all calculations work
    return area1 + area2 + perim1;  // Should return 71
}
