// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

// Test getters and setters (accessors)

class Temperature {
    _celsius: number;

    constructor(c: number) {
        this._celsius = c;
    }

    get celsius(): number {
        return this._celsius;
    }

    set celsius(value: number) {
        if (value < 0) {
            this._celsius = 0;
        } else {
            this._celsius = value;
        }
    }

    get doubled(): number {
        return this._celsius * 2;
    }

    set doubled(v: number) {
        this._celsius = v / 2;
    }

    get kelvinOffset(): number {
        return this._celsius + 273;
    }
}

class Counter {
    private _count: number = 0;

    get count(): number {
        return this._count;
    }

    set count(value: number) {
        this._count = value;
    }

    increment(): void {
        this.count = this.count + 1;  // uses setter/getter
    }
}

class ReadOnly {
    private _data: number;
    constructor(v: number) { this._data = v; }
    get value(): number { return this._data; }
}

function main(): number {
    // Basic getter
    const t = new Temperature(100);
    if (t.celsius !== 100) return 1;
    if (t.doubled !== 200) return 2;
    if (t.kelvinOffset !== 373) return 3;

    // Basic setter
    t.celsius = 0;
    if (t.celsius !== 0) return 4;
    if (t.doubled !== 0) return 5;

    // Setter with validation
    t.celsius = -500;  // below zero (test floor)
    if (t.celsius !== 0) return 6;

    // Setter via computed property
    const t2 = new Temperature(0);
    t2.doubled = 100;  // sets _celsius to 50
    if (t2.celsius !== 50) return 7;
    if (t2.doubled !== 100) return 8;  // reads _celsius*2

    // Setter chain via increment
    const ctr = new Counter();
    ctr.increment();
    ctr.increment();
    ctr.increment();
    if (ctr.count !== 3) return 9;

    // Direct setter access
    ctr.count = 100;
    if (ctr.count !== 100) return 10;

    // Read-only getter (no setter)
    const ro = new ReadOnly(42);
    if (ro.value !== 42) return 11;

    return 0;
}
