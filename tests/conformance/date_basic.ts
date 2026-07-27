// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

// Test Date — focus on deterministic, non-timezone-sensitive behavior

function main(): number {
    // Date.now() should be a positive integer (just sanity check)
    const now = Date.now();
    if (typeof now !== "number") return 1;
    if (now < 0) return 2;

    // Construct from year, month, day, hours, minutes
    const d1 = new Date(2024, 0, 15);  // January 15, 2024 (month=0-indexed)
    if (d1.getFullYear() !== 2024) return 3;
    if (d1.getMonth() !== 0) return 4;
    if (d1.getDate() !== 15) return 5;

    // With time
    const d2 = new Date(2024, 5, 10, 14, 30, 45);  // June 10, 2024 14:30:45
    if (d2.getHours() !== 14) return 6;
    if (d2.getMinutes() !== 30) return 7;
    if (d2.getSeconds() !== 45) return 8;

    // Day of week (June 10, 2024 is Monday = 1)
    if (d2.getDay() !== 1) return 9;

    // Construct from milliseconds
    const d3 = new Date(0);  // epoch
    if (d3.getTime() !== 0) return 10;
    if (d3.getFullYear() !== 1970) return 11;
    if (d3.getMonth() !== 0) return 12;  // January
    if (d3.getDate() !== 1) return 13;

    // Construct from string (ISO)
    const d4 = new Date("2024-06-15T12:00:00Z");
    if (d4.getUTCFullYear() !== 2024) return 14;
    if (d4.getUTCMonth() !== 5) return 15;  // June
    if (d4.getUTCDate() !== 15) return 16;
    if (d4.getUTCHours() !== 12) return 17;

    // Setters
    const d5 = new Date(2024, 0, 1);
    d5.setFullYear(2030);
    if (d5.getFullYear() !== 2030) return 18;

    d5.setMonth(11);  // December
    if (d5.getMonth() !== 11) return 19;

    d5.setDate(25);
    if (d5.getDate() !== 25) return 20;

    // getTime returns same as valueOf
    const d6 = new Date(2024, 0, 1);
    if (d6.getTime() !== d6.valueOf()) return 21;

    // Date.parse
    const parsed = Date.parse("1970-01-01T00:00:00Z");
    if (parsed !== 0) return 22;

    // Date.UTC
    const utc = Date.UTC(2024, 0, 1, 0, 0, 0);
    if (utc !== 1704067200000) return 23;

    // toString contains year
    const str = new Date(2024, 0, 1).toString();
    if (str.indexOf("2024") < 0) return 24;

    // toISOString
    const iso = new Date(Date.UTC(2024, 0, 1)).toISOString();
    if (iso.indexOf("2024-01-01") < 0) return 25;

    // Arithmetic
    const start = new Date(2024, 0, 1);
    const end = new Date(2024, 0, 11);
    const diff = end.getTime() - start.getTime();
    const days = Math.round(diff / (1000 * 60 * 60 * 24));
    if (days !== 10) return 26;

    return 0;
}
