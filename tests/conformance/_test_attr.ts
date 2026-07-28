function main(): number {
    let computed = { first: 1, second: 2 };
    console.log("before defineProperty:");
    console.log("  first=" + computed.first);
    console.log("  second=" + computed.second);
    Object.defineProperty(computed, "first", { writable: false });
    console.log("after defineProperty:");
    console.log("  first=" + computed.first);
    console.log("  second=" + computed.second);
    computed["first"] = 10;
    computed["second"] = 20;
    console.log("after assignments:");
    console.log("  first=" + computed.first);
    console.log("  second=" + computed.second);
    return 0;
}
