// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

// Test private fields (#field), private methods, and access control

class Account {
    #balance: number;
    readonly #owner: string;
    static #count: number = 0;

    constructor(owner: string, initial: number) {
        this.#owner = owner;
        this.#balance = initial;
        Account.#count = Account.#count + 1;
    }

    deposit(amount: number): void {
        if (amount > 0) {
            this.#balance += amount;
        }
    }

    withdraw(amount: number): boolean {
        if (this.#balance >= amount) {
            this.#balance -= amount;
            return true;
        }
        return false;
    }

    getBalance(): number {
        return this.#balance;
    }

    getOwner(): string {
        return this.#owner;
    }

    static totalAccounts(): number {
        return Account.#count;
    }
}

class Counter {
    #count: number = 0;

    increment(): void {
        this.#count++;
    }

    #reset(): void {
        this.#count = 0;
    }

    public reset(): void {
        this.#reset();
    }

    value(): number {
        return this.#count;
    }
}

function main(): number {
    // Basic private field
    const acc = new Account("alice", 100);
    if (acc.getBalance() !== 100) return 1;
    if (acc.getOwner() !== "alice") return 2;

    acc.deposit(50);
    if (acc.getBalance() !== 150) return 3;

    if (!acc.withdraw(40)) return 4;
    if (acc.getBalance() !== 110) return 5;

    // Withdraw more than balance
    if (acc.withdraw(1000)) return 6;
    if (acc.getBalance() !== 110) return 7;

    // Static private counter
    const before = Account.totalAccounts();
    const acc2 = new Account("bob", 200);
    if (Account.totalAccounts() !== before + 1) return 8;

    // Private method
    const ctr = new Counter();
    ctr.increment();
    ctr.increment();
    ctr.increment();
    if (ctr.value() !== 3) return 9;

    ctr.reset();  // calls private #reset()
    if (ctr.value() !== 0) return 10;

    // Two instances don't share private fields
    const a = new Account("a", 100);
    const b = new Account("b", 200);
    a.deposit(50);
    if (a.getBalance() !== 150) return 11;
    if (b.getBalance() !== 200) return 12;

    return 0;
}
