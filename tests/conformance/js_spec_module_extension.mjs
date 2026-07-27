// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

export function identity(value) {
    return value;
}

function main() {
    return identity(42) === 42 ? 0 : 1;
}
