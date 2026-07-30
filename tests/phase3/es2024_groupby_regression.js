// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main() {
    const grouped = Object.groupBy(
        [1, 2, 3, 4],
        (value) => value % 2 === 0 ? "even" : "odd"
    );
    if (grouped.even.join(",") !== "2,4") return 1;
    if (grouped.odd.join(",") !== "1,3") return 2;

    const mapped = Map.groupBy(
        ["a", "bb", "c"],
        (value) => value.length
    );
    const singles = mapped.get(1).join(",");
    const doubles = mapped.get(2).join(",");
    console.log("map-groups:" + singles + "|" + doubles);
    if (singles !== "a,c") return 3;
    return doubles === "bb" ? 0 : 4;
}
