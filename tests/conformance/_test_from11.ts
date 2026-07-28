function main(): number {
    console.log("start");
    let object = Object.fromEntries([
        ["first", 10],
        ["second", "Nova"],
        ["first", 30]
    ]);
    console.log("after fromEntries");
    if (object.first != 30) { console.log("FAIL1"); return 1; }
    console.log("past check1");
    return 0;
}
