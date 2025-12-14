function outer() {
    let x = 42;
    return function inner() {
        return x;
    };
}

const fn = outer();
fn();
