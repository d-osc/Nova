function main(): number {
    let object = Object.fromEntries([
        ["first", 10],
        ["second", "Nova"]
    ]);
    let keys = Object.keys(object);
    console.log("keys.length=" + keys.length);
    console.log("keys[0]=" + keys[0]);
    console.log("keys[1]=" + keys[1]);
    return 0;
}
