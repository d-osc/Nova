// Test private fields (#)
class Counter {
    #count: number;

    constructor() {
        this.#count = 0;
    }

    increment(): number {
        this.#count = this.#count + 1;
        return this.#count;
    }

    getCount(): number {
        return this.#count;
    }
}

function main(): number {
    let c = new Counter();
    c.increment();
    c.increment();
    let count = c.getCount();  // Should be 2

    if (count != 2) {
        return 1;
    }
    return 0;
}
