// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main() {
    const number = new Intl.NumberFormat("en-US", {
        style: "currency",
        currency: "USD"
    });
    if (number.format(1234.5) !== "$1,234.50") return 1;
    if (number.resolvedOptions().currency !== "USD") return 2;

    const date = new Intl.DateTimeFormat("en-CA", {
        timeZone: "UTC",
        year: "numeric",
        month: "2-digit",
        day: "2-digit"
    });
    const parts = date.formatToParts(new Date("2024-01-02T00:00:00Z"));
    if (!parts.some((part) => part.type === "year" && part.value === "2024")) return 3;

    const collator = new Intl.Collator("en", { sensitivity: "base" });
    if (collator.compare("a", "A") !== 0) return 4;

    const list = new Intl.ListFormat("en", { style: "long", type: "conjunction" });
    if (list.format(["A", "B", "C"]) !== "A, B, and C") return 5;

    const segmenter = new Intl.Segmenter("th", { granularity: "word" });
    if ([...segmenter.segment("ภาษาไทย")].length < 1) return 6;

    return 0;
}
