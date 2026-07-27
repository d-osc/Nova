// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main() {
    const grouped = Object.groupBy([1, 2, 3, 4], (value) =>
        value % 2 === 0 ? "even" : "odd"
    );
    if (grouped.even.join(",") !== "2,4") return 1;
    if (grouped.odd.join(",") !== "1,3") return 2;

    const mapped = Map.groupBy(["a", "bb", "c"], (value) => value.length);
    if (mapped.get(1).join(",") !== "a,c") return 3;
    if (mapped.get(2).join(",") !== "bb") return 4;

    const capability = Promise.withResolvers();
    if (!(capability.promise instanceof Promise)) return 5;
    if (typeof capability.resolve !== "function") return 6;
    if (typeof capability.reject !== "function") return 7;
    capability.resolve("ok");

    const loneHighSurrogate = "\uD800";
    if (loneHighSurrogate.isWellFormed()) return 8;
    if (!loneHighSurrogate.toWellFormed().isWellFormed()) return 9;

    const buffer = new ArrayBuffer(8, { maxByteLength: 16 });
    if (!buffer.resizable || buffer.maxByteLength !== 16) return 10;
    buffer.resize(12);
    if (buffer.byteLength !== 12) return 11;

    return 0;
}
