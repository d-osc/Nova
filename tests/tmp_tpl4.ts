function main(): number {
    const nested = `outer ${`inner ${1 + 1}`} end`;
    console.log(nested);
    return 0;
}
