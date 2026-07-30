/*---
description: Optional chaining reads an existing property
features: [optional-chaining]
includes: [assert.js]
esid: sec-optional-chaining
---*/
const value = { name: "nova" };
assert(value?.name === "nova");
