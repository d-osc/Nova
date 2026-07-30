/*---
description: Invalid const binding is a parse error
features: [destructuring-binding]
negative:
  phase: parse
  type: SyntaxError
esid: sec-let-and-const-declarations
---*/
const = 1;
