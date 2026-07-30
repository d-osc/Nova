// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    let computed = { first: 1, second: 2 };
    Object.defineProperty(computed, "first", { writable: false });
    // Direct assignment via dot notation
    computed.first = 10;
    computed.second = 20;
    console.log("dot-assigned: first=" + computed.first + " second=" + computed.second);
    return 0;
}
