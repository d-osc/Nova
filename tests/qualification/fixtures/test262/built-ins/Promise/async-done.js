/*---
description: Promise jobs complete through the async harness
features: [Promise]
flags: [async]
includes: [assert.js]
esid: sec-promise-objects
---*/
Promise.resolve(42).then((value) => {
    assert(value === 42);
    $DONE();
});
