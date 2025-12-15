console.log("Test: Rest parameters");
function sum(...numbers) {
  let total = 0;
  for (let i = 0; i < numbers.length; i++) {
    total = total + numbers[i];
  }
  return total;
}
const result = sum(1, 2, 3, 4, 5);
console.log("sum(1,2,3,4,5):", result);
console.log("Done");
