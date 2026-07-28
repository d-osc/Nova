function main(): number {
    let source = { visible: 10, hidden: 20 };
    Object.defineProperty(source, "hidden", { enumerable: false });
    let target = { visible: 1, hidden: 2 };
    let r = Object.assign(target, source);
    console.log("target.visible=" + target.visible);
    console.log("target.hidden=" + target.hidden);
    return 0;
}
