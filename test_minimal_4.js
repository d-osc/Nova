// Test 4: Object literal in loop (no functions)
console.log("Test 4: Object literal in loop");
var objects = [];
for (var i = 0; i < 1000; i++) {
    objects.push({ id: i, value: i * 2 });
}
console.log("Created " + objects.length + " objects");
