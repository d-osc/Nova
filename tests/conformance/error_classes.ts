// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

// Test Error classes and subclasses

class CustomError extends Error {
    code: number;

    constructor(message: string, code: number) {
        super(message);
        this.name = "CustomError";
        this.code = code;
    }
}

class ValidationError extends Error {
    field: string;

    constructor(field: string, message: string) {
        super(message);
        this.name = "ValidationError";
        this.field = field;
    }
}

class NotFoundError extends Error {
    resource: string;

    constructor(resource: string) {
        super(`${resource} not found`);
        this.name = "NotFoundError";
        this.resource = resource;
    }
}

function riskyOp(input: number): number {
    if (input < 0) {
        throw new ValidationError("input", "must be non-negative");
    }
    if (input === 0) {
        throw new NotFoundError("record");
    }
    if (input > 100) {
        throw new CustomError("too large", 400);
    }
    return input * 2;
}

function main(): number {
    // Successful call
    const result = riskyOp(5);
    if (result !== 10) return 1;

    // Catch by instanceof — ValidationError
    try {
        riskyOp(-1);
        return 2;  // should have thrown
    } catch (e) {
        if (!(e instanceof ValidationError)) return 3;
        if (!(e instanceof Error)) return 4;
        const err = e as ValidationError;
        if (err.field !== "input") return 5;
        if (err.message !== "must be non-negative") return 6;
        if (err.name !== "ValidationError") return 7;
    }

    // Catch by instanceof — NotFoundError
    try {
        riskyOp(0);
        return 8;
    } catch (e) {
        if (!(e instanceof NotFoundError)) return 9;
        const err = e as NotFoundError;
        if (err.resource !== "record") return 10;
        if (err.message.indexOf("record") < 0) return 11;
        if (err.name !== "NotFoundError") return 12;
    }

    // Catch by instanceof — CustomError
    try {
        riskyOp(200);
        return 13;
    } catch (e) {
        if (!(e instanceof CustomError)) return 14;
        const err = e as CustomError;
        if (err.code !== 400) return 15;
        if (err.message !== "too large") return 16;
    }

    // Built-in Error
    try {
        throw new Error("plain");
    } catch (e) {
        if (!(e instanceof Error)) return 17;
        const err = e as Error;
        if (err.message !== "plain") return 18;
        if (err.name !== "Error") return 19;
    }

    // TypeError, RangeError
    try {
        throw new TypeError("wrong type");
    } catch (e) {
        if (!(e instanceof TypeError)) return 20;
        if (!(e instanceof Error)) return 21;
        if ((e as TypeError).message !== "wrong type") return 22;
    }

    try {
        throw new RangeError("out of range");
    } catch (e) {
        if (!(e instanceof RangeError)) return 23;
        if (!(e instanceof Error)) return 24;
    }

    // Error with options.cause
    try {
        try {
            throw new Error("root cause");
        } catch (root) {
            throw new Error("wrapped", { cause: root });
        }
    } catch (e) {
        const err = e as any;
        if (err.message !== "wrapped") return 25;
        // cause should reference the original error
        const cause = err.cause;
        if (!cause) return 26;
        if (cause.message !== "root cause") return 27;
    }

    // Subclass instanceof chain
    class A extends Error {}
    class B extends A {}
    class C extends B {}
    const c = new C("test");
    if (!(c instanceof C)) return 28;
    if (!(c instanceof B)) return 29;
    if (!(c instanceof A)) return 30;
    if (!(c instanceof Error)) return 31;

    return 0;
}
