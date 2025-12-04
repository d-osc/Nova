import { dirname } from "path";

const input = "/home/user/test.txt";
console.log("Calling dirname with:", input);
const result = dirname(input);
console.log("Result from dirname:", result);
console.log("Expected: /home/user");
