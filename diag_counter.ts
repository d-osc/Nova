class Counter {
    #count: number = 0;

    increment(): void {
        this.#count++;
    }

    #reset(): void {
        this.#count = 0;
    }

    public reset(): void {
        this.#reset();
    }

    value(): number {
        return this.#count;
    }
}

function main(): number {
    const ctr = new Counter();
    ctr.increment();
    ctr.increment();
    ctr.increment();
    console.log("after increment: " + ctr.value());
    ctr.reset();
    console.log("after reset: " + ctr.value());
    return 0;
}
