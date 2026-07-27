// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

// Test try/catch/finally semantics

function thrower(msg: string): never {
    throw msg;
}

function risky(): number {
    throw "fail";
}

function safe(): number {
    return 42;
}

class MyError {
    name: string = "MyError";
    message: string;
    constructor(msg: string) {
        this.message = msg;
    }
}

function main(): number {
    // Basic try/catch
    let caught: string = "";
    try {
        throw "oops";
    } catch (e) {
        caught = e as string;
    }
    if (caught !== "oops") return 1;

    // try/finally (no catch)
    let finallyRan = false;
    try {
        // no-op
    } finally {
        finallyRan = true;
    }
    if (!finallyRan) return 2;

    // try/catch/finally — finally runs after catch
    let order: string[] = [];
    try {
        order.push("try");
        throw "x";
    } catch (e) {
        order.push("catch");
    } finally {
        order.push("finally");
    }
    if (order.length !== 3) return 3;
    if (order[0] !== "try" || order[1] !== "catch" || order[2] !== "finally") return 4;

    // Throw inside catch
    let outerCaught = "";
    try {
        try {
            throw "inner";
        } catch (e) {
            throw "rethrow";
        }
    } catch (e) {
        outerCaught = e as string;
    }
    if (outerCaught !== "rethrow") return 5;

    // finally with return override semantics (just test finally executes)
    let marker = 0;
    try {
        marker = 1;
    } catch (e) {
        marker = 100;
    } finally {
        marker += 10;
    }
    if (marker !== 11) return 6;

    // Typed error object
    let errMsg = "";
    try {
        throw new MyError("boom");
    } catch (e) {
        const err = e as MyError;
        errMsg = err.message;
    }
    if (errMsg !== "boom") return 7;

    // Function that throws, caught at call site
    let result = 0;
    try {
        result = risky();
        result = 999;  // should not reach
    } catch (e) {
        result = -1;
    }
    if (result !== -1) return 8;

    // No throw — try succeeds
    try {
        const v = safe();
        if (v !== 42) return 9;
    } catch (e) {
        return 10;  // should not catch
    }

    return 0;
}
