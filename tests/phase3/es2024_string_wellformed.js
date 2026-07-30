// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main() {
    const loneHighSurrogate = "\uD800";
    if (loneHighSurrogate.isWellFormed()) return 1;
    return loneHighSurrogate.toWellFormed().isWellFormed() ? 0 : 2;
}
