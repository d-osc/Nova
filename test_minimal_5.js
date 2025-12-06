// Test 5: One function + object literal
console.log("Test 5: One function + object literal");
function add(a, b) {
    return a + b;
}
var objects = [];
for (var i = 0; i < 1000; i++) {
    objects.push({ id: i, value: i * 2 });
}
console.log("Result: " + add(1, 2) + ", Objects: " + objects.length);
