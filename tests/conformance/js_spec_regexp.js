// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main() {
    const named = /(?<year>\d{4})-(?<month>\d{2})/;
    const match = named.exec("2024-07");
    if (!match || match.groups.year !== "2024" || match.groups.month !== "07") return 1;

    if (!/(?<=\$)\d+/.test("$42")) return 2;
    if (!/a.b/s.test("a\nb")) return 3;
    if (!/\p{Letter}+/u.test("ภาษาไทย")) return 4;

    const words = [..."a1 b22".matchAll(/(?<letter>[a-z])(?<number>\d+)/g)];
    if (words.length !== 2) return 5;
    if (words[1].groups.letter !== "b" || words[1].groups.number !== "22") return 6;

    const sticky = /\d/y;
    sticky.lastIndex = 1;
    if (!sticky.test("a1") || sticky.lastIndex !== 2) return 7;

    const indices = /a/d.exec("cat");
    if (!indices || indices.indices[0][0] !== 1 || indices.indices[0][1] !== 2) return 8;

    return 0;
}
