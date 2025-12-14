function outer() {
    let x = 42;
    function inner() {
        return x;
    }
    return inner;
}
