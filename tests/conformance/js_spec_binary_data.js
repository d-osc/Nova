// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main() {
    const buffer = new ArrayBuffer(16);
    if (buffer.byteLength !== 16) return 1;

    const view = new DataView(buffer);
    view.setUint16(0, 0x1234, false);
    view.setInt32(4, -42, true);
    view.setFloat64(8, Math.PI, true);
    if (view.getUint16(0, false) !== 0x1234) return 2;
    if (view.getInt32(4, true) !== -42) return 3;
    if (Math.abs(view.getFloat64(8, true) - Math.PI) > 1e-12) return 4;

    const bytes = new Uint8Array(buffer, 0, 4);
    if (bytes.byteLength !== 4 || bytes.buffer !== buffer) return 5;
    const copy = bytes.slice();
    copy[0] = 255;
    if (bytes[0] === 255) return 6;

    const shared = new SharedArrayBuffer(8);
    const integers = new Int32Array(shared);
    if (Atomics.store(integers, 0, 10) !== 10) return 7;
    if (Atomics.add(integers, 0, 5) !== 10) return 8;
    if (Atomics.load(integers, 0) !== 15) return 9;
    if (Atomics.compareExchange(integers, 0, 15, 20) !== 15) return 10;
    if (Atomics.load(integers, 0) !== 20) return 11;

    return 0;
}
