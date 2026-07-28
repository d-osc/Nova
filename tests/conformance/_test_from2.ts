function main(): number {
    let entries: [string, number][] = [["first", 10]];
    let obj = Object.fromEntries(entries);
    console.log("after fromEntries");
    console.log("obj.first=" + obj.first);
    return 0;
}
