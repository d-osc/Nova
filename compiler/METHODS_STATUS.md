# Nova Compiler - Implemented Methods Status

## ✅ String Methods (Working)
- `String.fromCharCode(code)` - Create string from character code (static) **[v0.91.0]**
- `String.prototype.charAt(index)` - Get character at index
- `String.prototype.charCodeAt(index)` - Get character code at index **[v0.90.0]**
- `String.prototype.concat(otherString)` - Concatenate strings **[v0.93.0 NEW!]**
- `String.prototype.substring(start, end)` - Extract substring
- `String.prototype.indexOf(searchString)` - Find first occurrence
- `String.prototype.lastIndexOf(searchString)` - Find last occurrence **[v0.89.0]**
- `String.prototype.toLowerCase()` - Convert to lowercase
- `String.prototype.toUpperCase()` - Convert to uppercase
- `String.prototype.trim()` - Remove whitespace
- `String.prototype.trimStart()` - Remove leading whitespace **[v0.94.0]**
- `String.prototype.trimEnd()` - Remove trailing whitespace **[v0.95.0 NEW!]**
- `String.prototype.startsWith(prefix)` - Check if starts with
- `String.prototype.endsWith(suffix)` - Check if ends with
- `String.prototype.repeat(count)` - Repeat string
- `String.prototype.includes(searchString)` - Check if contains
- `String.prototype.slice(start, end)` - Extract slice (supports negative indices)
- `String.prototype.replace(search, replace)` - Replace first occurrence
- `String.prototype.padStart(length, fillString)` - Pad at start
- `String.prototype.padEnd(length, fillString)` - Pad at end
- `String.prototype.split(delimiter)` - Split into array **[v0.74.0]**
- `String.prototype.length` - String length property

## ✅ Array Methods (Working)
- `Array.prototype.push(value)` - Add to end
- `Array.prototype.pop()` - Remove from end
- `Array.prototype.shift()` - Remove from start
- `Array.prototype.unshift(value)` - Add to start
- `Array.prototype.at(index)` - Get element at index (supports negative) **[v0.92.0 NEW!]**
- `Array.prototype.includes(value)` - Check if contains
- `Array.prototype.indexOf(value)` - Find first index
- `Array.prototype.lastIndexOf(value)` - Find last index **[v0.87.0]**
- `Array.prototype.reverse()` - Reverse in place
- `Array.prototype.fill(value)` - Fill with value
- `Array.prototype.join(delimiter)` - Join to string **[v0.75.0]**
- `Array.prototype.concat(otherArray)` - Concatenate arrays **[v0.76.0]**
- `Array.prototype.slice(start, end)` - Extract sub-array **[v0.77.0]**
- `Array.prototype.find(callback)` - Find first matching element **[v0.79.0]**
- `Array.prototype.findIndex(callback)` - Find first matching index **[v0.86.0]**
- `Array.prototype.filter(callback)` - Filter elements by condition **[v0.80.0]**
- `Array.prototype.map(callback)` - Transform each element **[v0.81.0]**
- `Array.prototype.some(callback)` - Check if any element matches **[v0.82.0]**
- `Array.prototype.every(callback)` - Check if all elements match **[v0.83.0]**
- `Array.prototype.forEach(callback)` - Iterate with callback **[v0.84.0]**
- `Array.prototype.reduce(callback, initialValue)` - Reduce to single value **[v0.85.0]**
- `Array.prototype.reduceRight(callback, initialValue)` - Reduce right-to-left **[v0.88.0]**
- `Array.prototype.length` - Array length property
- `Array.isArray(value)` - Check if value is array

## ✅ Math Methods (Working)
- `Math.abs(x)` - Absolute value
- `Math.ceil(x)` - Round up
- `Math.floor(x)` - Round down
- `Math.round(x)` - Round to nearest
- `Math.sqrt(x)` - Square root
- `Math.pow(base, exp)` - Power
- `Math.sign(x)` - Sign of number
- `Math.trunc(x)` - Truncate decimals
- `Math.cbrt(x)` - Cube root
- `Math.hypot(...values)` - Euclidean distance
- `Math.random()` - Random number
- `Math.fround(x)` - Round to float32
- `Math.imul(a, b)` - Integer multiplication
- `Math.clz32(x)` - Count leading zeros
- `Math.log(x)` - Natural logarithm (base e) **[v0.96.0]**
- `Math.exp(x)` - Exponential function (e^x) **[v0.97.0]**
- `Math.log10(x)` - Base 10 logarithm **[v0.98.0]**
- `Math.log2(x)` - Base 2 logarithm **[v0.99.0]**
- `Math.sin(x)` - Sine function (radians) **[v1.0.0]**
- `Math.cos(x)` - Cosine function (radians) **[v1.0.1]**
- `Math.tan(x)` - Tangent function (radians) **[v1.0.2]**
- `Math.atan(x)` - Arctangent / inverse tangent (radians) **[v1.0.3]**
- `Math.asin(x)` - Arcsine / inverse sine (radians) **[v1.0.4]**
- `Math.acos(x)` - Arccosine / inverse cosine (radians) **[v1.0.5]**
- `Math.atan2(y, x)` - Two-argument arctangent (radians) **[v1.0.6]**
- `Math.sinh(x)` - Hyperbolic sine function **[v1.0.7]**
- `Math.cosh(x)` - Hyperbolic cosine function **[v1.0.8]**
- `Math.tanh(x)` - Hyperbolic tangent function **[v1.0.9]**
- `Math.asinh(x)` - Inverse hyperbolic sine function **[v1.1.0 NEW!]**

## ✅ Number Methods (Working)
- `Number.isFinite(value)` - Check if finite
- `Number.isNaN(value)` - Check if NaN
- `Number.isInteger(value)` - Check if integer
- `Number.isSafeInteger(value)` - Check if safe integer

## ✅ Operators & Language Features (Working)
- `typeof` - Type checking operator
- `instanceof` - Instance checking operator
- Ternary operator `? :`
- Logical operators `&&`, `||`, `!`
- Bitwise operators `&`, `|`, `^`, `~`, `<<`, `>>`, `>>>`
- Increment/Decrement `++`, `--`
- Template literals
- Arrow functions (basic support)
- Classes (basic support)
- `console.log()` - Console output

## ⚠️ Known Limitations
1. **Type Inference**: ~~Array methods that return new arrays (concat, slice) lose type info when stored in variables~~ **FIXED in v0.78.0** ✅
   - ~~Workaround: Access `.length` on original array or use inline~~
2. **Callbacks**:
   - ✅ Array.find() with arrow function callbacks **[v0.79.0]**
   - ✅ Array.findIndex() with arrow function callbacks **[v0.86.0]**
   - ✅ Array.filter() with arrow function callbacks **[v0.80.0]**
   - ✅ Array.map() with arrow function callbacks **[v0.81.0]**
   - ✅ Array.some() with arrow function callbacks **[v0.82.0]**
   - ✅ Array.every() with arrow function callbacks **[v0.83.0]**
   - ✅ Array.forEach() with arrow function callbacks **[v0.84.0]**
   - ✅ Array.reduce() with 2-parameter arrow function callbacks **[v0.85.0]**
   - ✅ Array.reduceRight() with 2-parameter arrow function callbacks **[v0.88.0]**
   - Note: Closures not yet supported (callbacks can't capture outer variables)
3. **Async**: Promise/async/await not implemented

## 📊 Statistics
- **Total Methods**: 71+ methods implemented
- **Latest Version**: v1.1.0 🎉
- **Test Suite**: 208/208 tests passing (100%) ✅
- **Test Runner**: `run_all_tests.py` available
- **String Methods**: 21+
- **Array Methods**: 24+ (with 9 callback methods!)
- **Math Methods**: 29+ (trig + inverse trig + hyperbolic + inverse hyperbolic!)
- **Number Methods**: 4+

