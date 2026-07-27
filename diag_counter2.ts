class Counter {
    #count: number = 0;

    public reset(): void {
        console.log("reset called");
        this.#reset();
        console.log("#reset done");
    }

    #reset(): void {
        console.log("inside #reset");
        this.#count = 0;
    }

    value(): number {
        return this.#count;
    }

    increment(): void {
        this.#count++;
    }
}

function main(): number {
    const ctr = new Counter();
    ctr.increment();
    ctr.increment();
    console.log("value before reset: " + ctr.value());
    ctr.reset();
    console.log("value after reset: " + ctr.value());
    return 0;
}
