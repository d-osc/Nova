// Test string operations
console.log("Testing string operations:");

const str1 = "hello";
console.log("str1 =", str1);
console.log("str1 === \"hello\" =", str1 === "hello");

const s1 = "Hello";
const s2 = "World";
const s3 = s1 + " " + s2;
console.log("\ns1 =", s1);
console.log("s2 =", s2);
console.log("s3 = s1 + \" \" + s2 =", s3);
console.log("s3 === \"Hello World\" =", s3 === "Hello World");

const name = "Nova";
const version = "1.4.0";
const msg1 = `${name} v${version}`;
console.log("\nname =", name);
console.log("version =", version);
console.log("msg1 = `${name} v${version}` =", msg1);
console.log("msg1 === \"Nova v1.4.0\" =", msg1 === "Nova v1.4.0");
