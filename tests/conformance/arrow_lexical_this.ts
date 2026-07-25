// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "arrow-this:7"

class Box {
    value: number;

    constructor(value: number) {
        this.value = value;
    }

    readViaArrow(): number {
        let reader = (): number => this.value;
        return reader();
    }
}

function main(): number {
    let box = new Box(7);
    console.log("arrow-this:" + box.readViaArrow());
    return 0;
}
