// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main() {
    const buffer = new ArrayBuffer(8, { maxByteLength: 16 });
    if (!buffer.resizable || buffer.maxByteLength !== 16) return 1;
    buffer.resize(12);
    return buffer.byteLength === 12 ? 0 : 2;
}
