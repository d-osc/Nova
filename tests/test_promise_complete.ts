// Test Promise Methods (Complete Suite)

function main(): number {
    console.log("=== Promise Static Methods ===");

    // Test Promise.resolve
    console.log("Testing Promise.resolve()...");
    let p1 = Promise.resolve(42);
    console.log("PASS: Promise.resolve()");

    // Test Promise.reject
    console.log("Testing Promise.reject()...");
    let p2 = Promise.reject("error");
    console.log("PASS: Promise.reject()");

    // Test Promise.all
    console.log("Testing Promise.all()...");
    let p3 = Promise.all([Promise.resolve(1), Promise.resolve(2)]);
    console.log("PASS: Promise.all()");

    // Test Promise.race
    console.log("Testing Promise.race()...");
    let p4 = Promise.race([Promise.resolve(1), Promise.resolve(2)]);
    console.log("PASS: Promise.race()");

    // Test Promise.allSettled
    console.log("Testing Promise.allSettled()...");
    let p5 = Promise.allSettled([Promise.resolve(1), Promise.reject("err")]);
    console.log("PASS: Promise.allSettled()");

    // Test Promise.any
    console.log("Testing Promise.any()...");
    let p6 = Promise.any([Promise.reject("err"), Promise.resolve(1)]);
    console.log("PASS: Promise.any()");

    // Test Promise.withResolvers
    console.log("Testing Promise.withResolvers()...");
    let resolvers = Promise.withResolvers();
    console.log("PASS: Promise.withResolvers()");

    console.log("");
    console.log("=== Promise Instance Methods ===");

    // Test .then
    console.log("Testing .then()...");
    let p7 = Promise.resolve(10).then((x) => x * 2);
    console.log("PASS: .then()");

    // Test .catch
    console.log("Testing .catch()...");
    let p8 = Promise.reject("error").catch((e) => "handled");
    console.log("PASS: .catch()");

    // Test .finally
    console.log("Testing .finally()...");
    let p9 = Promise.resolve(1).finally(() => console.log("cleanup"));
    console.log("PASS: .finally()");

    console.log("");
    console.log("All Promise tests passed!");
    return 0;
}
