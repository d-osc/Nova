console.log("Test 1: Arrow function");
const addArrow = (a, b) => a + b;
const resultArrow = addArrow(5, 3);
console.log("Arrow result:", resultArrow);

console.log("Test 2: Regular function");
function addRegular(a, b) {
  return a + b;
}
const resultRegular = addRegular(5, 3);
console.log("Regular result:", resultRegular);
