function main(): number {
    let target = { first: 1, second: 2 };
    let source = { first: 10, second: 20 };
    let result = Object.assign(target, source);
    console.log("target.first=" + target.first);
    console.log("target.second=" + target.second);
    console.log("result.first=" + result.first);
    console.log("result.second=" + result.second);
    return 0;
}
