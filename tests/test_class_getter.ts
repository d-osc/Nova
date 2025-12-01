// Test getter and setter

class Box {
    _value: number;

    constructor(v: number) {
        this._value = v;
    }

    get value(): number {
        return this._value;
    }

    set value(v: number) {
        this._value = v;
    }
}

function main(): number {
    let box = new Box(10);
    let v = box.value;  // getter

    if (v != 10) {
        return 1;
    }

    box.value = 20;  // setter
    let v2 = box.value;

    if (v2 != 20) {
        return 2;
    }

    return 0;
}
