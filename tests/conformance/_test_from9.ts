function main(): number {
    let object = Object.fromEntries([
        ["first", 10],
        ["second", "Nova"],
        ["first", 30]
    ]);
    if (object.first != 30) return 1;
    if (object.second != "Nova") return 2;
    if (object["first"] != 30) return 3;
    if (!Object.hasOwn(object, "first")) return 4;
    let keys = Object.keys(object);
    if (keys.length != 2) return 5;
    console.log("past length check");
    if (keys[0] != "first") return 6;
    console.log("past keys[0]");
    if (keys[1] != "second") return 7;
    console.log("past keys[1]");
    let empty = Object.fromEntries([]);
    console.log("created empty");
    if (Object.keys(empty).length != 0) return 8;
    console.log("done");
    return 0;
}
