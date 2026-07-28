function main(): number {
    let object = Object.fromEntries([
        ["first", 10],
        ["second", "Nova"],
        ["first", 30]
    ]);
    console.log("after fromEntries");
    console.log("object.first=" + object.first);
    console.log("object.second=" + object.second);
    return 0;
}
