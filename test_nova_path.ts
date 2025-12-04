import { dirname } from "nova:path";

const input = "/home/user/test.txt";
console.log("Input:", input);
const result = dirname(input);
console.log("Result:", result);
