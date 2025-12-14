// Test if nested function calls work
function inner() {
    return 42;
}

function outer(x) {
    console.log(x);
}

// This uses inner() result as argument to outer()
outer(inner());
