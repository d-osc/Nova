console.log("Testing section 5: Object creation");
var objects = [];
for (var i = 0; i < 100000; i++) {
    objects.push({ id: i, value: i * 2 });
}
console.log("Objects: " + objects.length);
console.log("Section 5 passed!");
