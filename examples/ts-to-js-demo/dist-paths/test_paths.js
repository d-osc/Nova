
const { User } = require("./src/models/User");
const { Product } = require("./src/models/Product");
const { unique, capitalize } = require("./src/utils/helpers");

const user = new User(1, "Test", "test@example.com");
console.log(user.getDisplayName());

const nums = unique([1, 2, 2, 3]);
console.log(nums);
