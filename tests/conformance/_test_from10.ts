function main(): number {
    console.log("start");
    let object = Object.fromEntries([
        ["first", 10],
        ["second", "Nova"],
        ["first", 30]
    ]);
    console.log("after fromEntries");
    return 0;
}
