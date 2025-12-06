console.log("Test object creation:");
var objects = [];
for (var i = 0; i < 1000; i++) {
    objects.push({ id: i, value: i * 2 });
}
console.log("Objects created: " + objects.length);
console.log("Done");
