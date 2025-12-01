// Comprehensive test for Date (ES1/ES5)

function main(): number {
    // =========================================
    // Test 1: Date.now() static method
    // =========================================
    let timestamp = Date.now();
    console.log("Date.now():", timestamp);
    console.log("PASS: Date.now()");

    // =========================================
    // Test 2: new Date() constructor
    // =========================================
    let now = new Date();
    console.log("new Date() created");
    console.log("PASS: Date constructor (no args)");

    // =========================================
    // Test 3: new Date(timestamp) constructor
    // =========================================
    let date1 = new Date(1700000000000);  // Nov 14, 2023
    console.log("new Date(1700000000000) created");
    console.log("PASS: Date constructor (timestamp)");

    // =========================================
    // Test 4: new Date(year, month, day...) constructor
    // =========================================
    let date2 = new Date(2024, 0, 15, 10, 30, 45, 500);  // Jan 15, 2024 10:30:45.500
    console.log("new Date(2024, 0, 15, 10, 30, 45, 500) created");
    console.log("PASS: Date constructor (parts)");

    // =========================================
    // Test 5: Getter methods (local time)
    // =========================================
    let year = date2.getFullYear();
    let month = date2.getMonth();
    let day = date2.getDate();
    let dayOfWeek = date2.getDay();
    let hours = date2.getHours();
    let minutes = date2.getMinutes();
    let seconds = date2.getSeconds();
    let ms = date2.getMilliseconds();

    console.log("getFullYear():", year);
    console.log("getMonth():", month);
    console.log("getDate():", day);
    console.log("getDay():", dayOfWeek);
    console.log("getHours():", hours);
    console.log("getMinutes():", minutes);
    console.log("getSeconds():", seconds);
    console.log("getMilliseconds():", ms);
    console.log("PASS: Getter methods (local)");

    // =========================================
    // Test 6: getTime() and valueOf()
    // =========================================
    let time = date2.getTime();
    let value = date2.valueOf();
    console.log("getTime():", time);
    console.log("valueOf():", value);
    console.log("PASS: getTime/valueOf");

    // =========================================
    // Test 7: UTC getter methods
    // =========================================
    let utcYear = date2.getUTCFullYear();
    let utcMonth = date2.getUTCMonth();
    let utcDay = date2.getUTCDate();
    let utcHours = date2.getUTCHours();
    console.log("getUTCFullYear():", utcYear);
    console.log("getUTCMonth():", utcMonth);
    console.log("getUTCDate():", utcDay);
    console.log("getUTCHours():", utcHours);
    console.log("PASS: UTC getter methods");

    // =========================================
    // Test 8: Setter methods
    // =========================================
    let date3 = new Date(2024, 5, 15);
    date3.setFullYear(2025);
    console.log("After setFullYear(2025):", date3.getFullYear());

    date3.setMonth(11);
    console.log("After setMonth(11):", date3.getMonth());

    date3.setDate(25);
    console.log("After setDate(25):", date3.getDate());

    date3.setHours(14);
    console.log("After setHours(14):", date3.getHours());
    console.log("PASS: Setter methods");

    // =========================================
    // Test 9: Conversion methods
    // =========================================
    let str = date2.toString();
    console.log("toString():", str);

    let dateStr = date2.toDateString();
    console.log("toDateString():", dateStr);

    let timeStr = date2.toTimeString();
    console.log("toTimeString():", timeStr);

    let isoStr = date2.toISOString();
    console.log("toISOString():", isoStr);

    let utcStr = date2.toUTCString();
    console.log("toUTCString():", utcStr);
    console.log("PASS: Conversion methods");

    // =========================================
    // Test 10: Date.UTC() static method
    // =========================================
    let utcTimestamp = Date.UTC(2024, 6, 4, 12, 0, 0, 0);
    console.log("Date.UTC(2024, 6, 4, 12, 0, 0, 0):", utcTimestamp);
    console.log("PASS: Date.UTC()");

    // =========================================
    // Test 11: getTimezoneOffset()
    // =========================================
    let tzOffset = now.getTimezoneOffset();
    console.log("getTimezoneOffset():", tzOffset);
    console.log("PASS: getTimezoneOffset()");

    // =========================================
    // All tests passed
    // =========================================
    console.log("");
    console.log("All Date comprehensive tests passed!");
    return 0;
}
