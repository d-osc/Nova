// Ultra minimal: one function + one object in loop (1 iter)
function f() { return 1; }
for (var i = 0; i < 1; i++) {
    var obj = { x: i };
}
console.log("done");
