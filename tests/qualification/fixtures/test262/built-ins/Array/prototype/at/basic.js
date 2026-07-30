/*---
description: Array.prototype.at reads positive and negative offsets
features: [Array.prototype.at]
includes: [assert.js]
esid: sec-array.prototype.at
---*/
const first = [10, 20, 30].at(0);
const last = [10, 20, 30].at(-1);
assert(first === 10);
assert(last === 30);
