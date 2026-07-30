// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main() {
    const formatter = new Intl.DateTimeFormat("en-CA", {
        timeZone: "UTC",
        year: "numeric",
        month: "2-digit",
        day: "2-digit"
    });
    const parts = formatter.formatToParts(
        new Date("2024-01-02T00:00:00Z"));
    if (parts.length !== 5) return 1;
    if (parts[4].type !== "year" || parts[4].value !== "2024") return 2;
    if (!parts.some((part) => part.value === "2024")) return 3;
    return 0;
}
