// Test Intl (Internationalization API)

function main(): number {
    // =========================================
    // Test 1: Intl.NumberFormat
    // =========================================
    console.log("=== Intl.NumberFormat ===");
    let nf = new Intl.NumberFormat("en-US");
    console.log("Created NumberFormat for en-US");
    let formatted = nf.format(1234567.89);
    console.log("Formatted 1234567.89:", formatted);
    console.log("PASS: Intl.NumberFormat");

    // =========================================
    // Test 2: Intl.DateTimeFormat
    // =========================================
    console.log("");
    console.log("=== Intl.DateTimeFormat ===");
    let dtf = new Intl.DateTimeFormat("en-US");
    console.log("Created DateTimeFormat for en-US");
    let date = new Date();
    let dateStr = dtf.format(date);
    console.log("Formatted current date:", dateStr);
    console.log("PASS: Intl.DateTimeFormat");

    // =========================================
    // Test 3: Intl.Collator
    // =========================================
    console.log("");
    console.log("=== Intl.Collator ===");
    let collator = new Intl.Collator("en");
    console.log("Created Collator for en");
    let cmp1 = collator.compare("a", "b");
    console.log("compare('a', 'b'):", cmp1);
    let cmp2 = collator.compare("b", "a");
    console.log("compare('b', 'a'):", cmp2);
    let cmp3 = collator.compare("a", "a");
    console.log("compare('a', 'a'):", cmp3);
    console.log("PASS: Intl.Collator");

    // =========================================
    // Test 4: Intl.PluralRules
    // =========================================
    console.log("");
    console.log("=== Intl.PluralRules ===");
    let pr = new Intl.PluralRules("en");
    console.log("Created PluralRules for en");
    let sel1 = pr.select(0);
    console.log("select(0):", sel1);
    let sel2 = pr.select(1);
    console.log("select(1):", sel2);
    let sel3 = pr.select(2);
    console.log("select(2):", sel3);
    console.log("PASS: Intl.PluralRules");

    // =========================================
    // Test 5: Intl.RelativeTimeFormat
    // =========================================
    console.log("");
    console.log("=== Intl.RelativeTimeFormat ===");
    let rtf = new Intl.RelativeTimeFormat("en");
    console.log("Created RelativeTimeFormat for en");
    let rel1 = rtf.format(-1, "day");
    console.log("format(-1, 'day'):", rel1);
    let rel2 = rtf.format(1, "day");
    console.log("format(1, 'day'):", rel2);
    console.log("PASS: Intl.RelativeTimeFormat");

    // =========================================
    // Test 6: Intl.ListFormat
    // =========================================
    console.log("");
    console.log("=== Intl.ListFormat ===");
    let lf = new Intl.ListFormat("en");
    console.log("Created ListFormat for en");
    // Note: ListFormat.format takes an array, simplified test
    console.log("PASS: Intl.ListFormat (basic)");

    // =========================================
    // Test 7: Intl.Locale
    // =========================================
    console.log("");
    console.log("=== Intl.Locale ===");
    let locale = new Intl.Locale("en-US");
    console.log("Created Locale for en-US");
    let locStr = locale.toString();
    console.log("toString():", locStr);
    console.log("PASS: Intl.Locale");

    // =========================================
    // Test 8: Intl.Segmenter
    // =========================================
    console.log("");
    console.log("=== Intl.Segmenter ===");
    let segmenter = new Intl.Segmenter("en");
    console.log("Created Segmenter for en");
    console.log("PASS: Intl.Segmenter (basic)");

    // =========================================
    // Test 9: Static methods
    // =========================================
    console.log("");
    console.log("=== Static Methods ===");
    let canonical = Intl.getCanonicalLocales("EN-us");
    console.log("getCanonicalLocales('EN-us'):", canonical);
    let supported = Intl.supportedValuesOf("calendar");
    console.log("supportedValuesOf('calendar') returned array");
    console.log("PASS: Static methods");

    // =========================================
    // All tests passed
    // =========================================
    console.log("");
    console.log("All Intl tests passed!");
    return 0;
}
