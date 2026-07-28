function main(): number {
    let object = Object.fromEntries([
        ["first", 10],
        ["second", "Nova"]
    ]);
    let keys = Object.keys(object);
    console.log("after keys");
    return 0;
}
