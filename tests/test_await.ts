// Test await expression (ES2017)
// Note: Runs synchronously, async runtime not yet available

async function getValue(): number {
    return 42;
}

async function testAwait(): number {
    // await would normally wait for promise
    let value: number = await getValue();
    return value;
}

function main(): number {
    let result: number = testAwait();

    if (result != 42) {
        return 1;
    }

    return 0;
}
