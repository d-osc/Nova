function main(): number {
    let protectedTarget = { first: 1, second: 2 };
    Object.defineProperty(protectedTarget, "first", { writable: false });
    console.log("after defineProperty:");
    console.log("  first=" + protectedTarget.first);
    console.log("  second=" + protectedTarget.second);
    let assignResult = Object.assign(
        protectedTarget,
        { first: 10, second: 20 }
    );
    console.log("after assign:");
    console.log("  first=" + protectedTarget.first);
    console.log("  second=" + protectedTarget.second);
    console.log("  result.first=" + assignResult.first);
    console.log("  result.second=" + assignResult.second);
    return 0;
}
