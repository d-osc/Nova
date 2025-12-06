// Test: function + small loop with object literals
console.log("Test 7: Function + small loop");
function add(a, b) {
    return a + b;
}
var objects = [];
for (var i = 0; i < 10; i++) {
    objects.push({ id: i, value: i * 2 });
}
console.log("Created: " + objects.length);
