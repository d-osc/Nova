function sum(...nums) {
    let total = 0;
    for (let i = 0; i < nums.length; i++) {
        total = total + nums[i];
    }
    return total;
}
console.log("Rest params:", sum(1, 2, 3));
