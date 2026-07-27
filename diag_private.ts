class Account {
    static #count: number = 0;
    constructor() {
        Account.#count = Account.#count + 1;
    }
    static totalAccounts(): number {
        return Account.#count;
    }
}

function main(): number {
    const before = Account.totalAccounts();
    console.log("before=" + before);
    const a = new Account();
    const after = Account.totalAccounts();
    console.log("after=" + after);
    if (after !== before + 1) {
        console.log("FAIL: after=" + after + " expected=" + (before + 1));
        return 8;
    }
    return 0;
}
