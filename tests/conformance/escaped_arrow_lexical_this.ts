// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "escaped-arrow-this:9"

class Box {
    value: number;

    constructor(value: number) {
        this.value = value;
    }

    makeReader() {
        return (): number => this.value;
    }
}

function main(): number {
    let box = new Box(9);
    let reader = box.makeReader();
    console.log("escaped-arrow-this:" + reader());
    return 0;
}
