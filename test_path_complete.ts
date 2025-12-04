import { dirname, basename, extname } from "nova:path";

const path = "/home/user/docs/file.txt";
console.log("Path:", path);
console.log("dirname:", dirname(path));
console.log("basename:", basename(path));
console.log("extname:", extname(path));
console.log("All Path functions working!");
