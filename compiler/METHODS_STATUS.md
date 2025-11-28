# Nova Compiler - Implemented Methods Status

## ✅ String Methods (Working)
- `String.fromCharCode(code)` - Create string from character code (static) **[v0.91.0]**
- `String.fromCodePoint(codePoint)` - Create string from Unicode code point (static, ES2015) **[v1.3.2]**
- `String.prototype.charAt(index)` - Get character at index
- `String.prototype.charCodeAt(index)` - Get character code at index **[v0.90.0]**
- `String.prototype.codePointAt(index)` - Get Unicode code point at index (ES2015) **[v1.3.1]**
- `String.prototype.at(index)` - Get character at index (supports negative) **[v1.1.5]**
- `String.prototype.concat(otherString)` - Concatenate strings **[v0.93.0]**
- `String.prototype.substring(start, end)` - Extract substring
- `String.prototype.indexOf(searchString)` - Find first occurrence
- `String.prototype.lastIndexOf(searchString)` - Find last occurrence **[v0.89.0]**
- `String.prototype.toLowerCase()` - Convert to lowercase
- `String.prototype.toUpperCase()` - Convert to uppercase
- `String.prototype.trim()` - Remove whitespace
- `String.prototype.trimStart()` - Remove leading whitespace **[v0.94.0]**
- `String.prototype.trimEnd()` - Remove trailing whitespace **[v0.95.0]**
- `String.prototype.startsWith(prefix)` - Check if starts with
- `String.prototype.endsWith(suffix)` - Check if ends with
- `String.prototype.repeat(count)` - Repeat string
- `String.prototype.includes(searchString)` - Check if contains
- `String.prototype.slice(start, end)` - Extract slice (supports negative indices)
- `String.prototype.replace(search, replace)` - Replace first occurrence
- `String.prototype.replaceAll(search, replace)` - Replace all occurrences (ES2021) **[v1.1.8]**
- `String.prototype.padStart(length, fillString)` - Pad at start
- `String.prototype.padEnd(length, fillString)` - Pad at end
- `String.prototype.split(delimiter)` - Split into array **[v0.74.0]**
- `String.prototype.length` - String length property

## ✅ Array Methods (Working)
- `Array.prototype.push(value)` - Add to end
- `Array.prototype.pop()` - Remove from end
- `Array.prototype.shift()` - Remove from start
- `Array.prototype.unshift(value)` - Add to start
- `Array.prototype.at(index)` - Get element at index (supports negative) **[v0.92.0]**
- `Array.prototype.with(index, value)` - Return copy with element replaced (ES2023) **[v1.1.9]**
- `Array.prototype.toReversed()` - Return reversed copy (immutable) (ES2023) **[v1.2.0]**
- `Array.prototype.toSorted()` - Return sorted copy (immutable, ascending) (ES2023) **[v1.2.1]**
- `Array.prototype.sort()` - Sort in place (mutable, ascending) **[v1.2.2]**
- `Array.prototype.splice(start, deleteCount)` - Remove elements in place **[v1.2.3]**
- `Array.prototype.toSpliced(start, deleteCount)` - Return copy with elements removed (immutable) (ES2023) **[v1.3.0]**
- `Array.prototype.copyWithin(target, start, end)` - Copy part to another location (ES2015) **[v1.2.9]**
- `Array.prototype.toString()` - Convert to comma-separated string **[v1.2.4]**
- `Array.prototype.flat()` - Flatten nested arrays one level deep (ES2019) **[v1.2.5]**
- `Array.prototype.flatMap(callback)` - Map then flatten one level (ES2019) **[v1.2.6]**
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
- `Array.prototype.findLast(callback)` - Find last matching element (ES2023) **[v1.1.6]**
- `Array.prototype.findLastIndex(callback)` - Find last matching index (ES2023) **[v1.1.7]**
- `Array.prototype.filter(callback)` - Filter elements by condition **[v0.80.0]**
- `Array.prototype.map(callback)` - Transform each element **[v0.81.0]**
- `Array.prototype.some(callback)` - Check if any element matches **[v0.82.0]**
- `Array.prototype.every(callback)` - Check if all elements match **[v0.83.0]**
- `Array.prototype.forEach(callback)` - Iterate with callback **[v0.84.0]**
- `Array.prototype.reduce(callback, initialValue)` - Reduce to single value **[v0.85.0]**
- `Array.prototype.reduceRight(callback, initialValue)` - Reduce right-to-left **[v0.88.0]**
- `Array.prototype.length` - Array length property
- `Array.isArray(value)` - Check if value is array (static)
- `Array.from(arrayLike)` - Create array from array-like object (static, ES2015) **[v1.2.7]**
- `Array.of(...elements)` - Create array from arguments (static, ES2015) **[v1.2.8 NEW!]**

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
- `Math.log1p(x)` - Returns ln(1 + x) (precise for small x) **[v1.1.4 NEW!]**
- `Math.exp(x)` - Exponential function (e^x) **[v0.97.0]**
- `Math.expm1(x)` - Returns e^x - 1 (precise for small x) **[v1.1.3]**
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
- `Math.asinh(x)` - Inverse hyperbolic sine function **[v1.1.0]**
- `Math.acosh(x)` - Inverse hyperbolic cosine function **[v1.1.1]**
- `Math.atanh(x)` - Inverse hyperbolic tangent function **[v1.1.2]**
- `Math.LN2` - Natural logarithm of 2 constant ≈ 0.693 **[v1.3.27]**
- `Math.LN10` - Natural logarithm of 10 constant ≈ 2.303 **[v1.3.28]**
- `Math.LOG2E` - Base 2 logarithm of E constant ≈ 1.443 **[v1.3.29]**
- `Math.LOG10E` - Base 10 logarithm of E constant ≈ 0.434 **[v1.3.30]**
- `Math.SQRT1_2` - Square root of 1/2 constant ≈ 0.707 **[v1.3.31]**
- `Math.SQRT2` - Square root of 2 constant ≈ 1.414 **[v1.3.32 NEW!]**

## ✅ Number Methods (Working)
- `Number.isFinite(value)` - Check if finite
- `Number.isNaN(value)` - Check if NaN
- `Number.isInteger(value)` - Check if integer
- `Number.isSafeInteger(value)` - Check if safe integer
- `Number.parseInt(string, radix)` - Parse string and return integer (static, ES5) **[v1.3.17]**
- `Number.parseFloat(string)` - Parse string and return floating-point number (static, ES5) **[v1.3.18 NEW!]**
- `Number.prototype.toFixed(digits)` - Format number with fixed decimal places **[v1.3.12]**
- `Number.prototype.toExponential(fractionDigits)` - Format number in exponential notation **[v1.3.13]**
- `Number.prototype.toPrecision(precision)` - Format number with specified precision **[v1.3.14]**
- `Number.prototype.toString(radix)` - Convert number to string with optional radix (base 2-36) **[v1.3.15]**
- `Number.prototype.valueOf()` - Return primitive value of Number object **[v1.3.16]**

## ✅ Object Methods (Working)
- `Object.isSealed(obj)` - Checks if object is sealed (static, ES5) **[v1.3.11]**
- `Object.seal(obj)` - Seals object, prevents add/delete properties (static, ES5) **[v1.3.10]**
- `Object.isFrozen(obj)` - Checks if object is frozen (static, ES5) **[v1.3.9]**
- `Object.freeze(obj)` - Makes object immutable (static, ES5) **[v1.3.8]**
- `Object.hasOwn(obj, key)` - Checks if object has own property (static, ES2022) **[v1.3.7]**
- `Object.assign(target, source)` - Copies properties from source to target (static, ES2015) **[v1.3.6]**
- `Object.entries(obj)` - Returns array of [key, value] pairs (static, ES2017) **[v1.3.5]**
- `Object.keys(obj)` - Returns array of object's property keys (static, ES2015) **[v1.3.4]**
- `Object.values(obj)` - Returns array of object's property values (static, ES2017) **[v1.3.3]**

## ✅ Global Functions (Working)
- `isNaN(value)` - Tests if value is NaN after coercing to number (global) **[v1.3.19]**
- `isFinite(value)` - Tests if value is finite after coercing to number (global) **[v1.3.20]**
- `parseInt(string, radix)` - Parses string to integer with optional radix (global, ES1) **[v1.3.25]**
- `parseFloat(string)` - Parses string to floating-point number (global, ES1) **[v1.3.26]**

## ✅ Console Methods (Working)
- `console.log(message)` - Outputs message to stdout
- `console.error(message)` - Outputs error message to stderr **[v1.3.21]**
- `console.warn(message)` - Outputs warning message to stderr **[v1.3.22]**
- `console.info(message)` - Outputs informational message to stdout **[v1.3.23]**
- `console.debug(message)` - Outputs debug message to stdout **[v1.3.24]**
- `console.clear()` - Clears the console using ANSI escape codes **[v1.3.33]**
- `console.time(label)` - Starts a timer with label for performance measurement **[v1.3.34]**
- `console.timeEnd(label)` - Stops timer and prints elapsed time in milliseconds **[v1.3.34]**
- `console.assert(condition, message)` - Prints error to stderr if condition is false **[v1.3.35]**
- `console.count(label)` - Increments and prints counter for label **[v1.3.36]**
- `console.countReset(label)` - Resets counter to zero **[v1.3.36]**
- `console.table(data)` - Displays array data in tabular format **[v1.3.37]**
- `console.group(label)` - Starts a new indented group with label **[v1.3.38]**
- `console.groupEnd()` - Ends the current group **[v1.3.38 NEW!]**

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
- **Total Methods**: 132+ methods implemented
- **Latest Version**: v1.3.38
- **Test Suite**: 274/274 tests passing (100%) ✅
- **Test Runner**: `run_all_tests.py` available
- **String Methods**: 25+ (includes ES2015, ES2021 & ES2022!)
- **Array Methods**: 38+ (with 12 callback methods! includes ES2015, ES2019 & ES2023 features!)
- **Math Methods/Constants**: 39+ (trig + inverse trig + hyperbolic + inverse hyperbolic + precision + ALL standard constants!)
- **Number Methods**: 11+ (complete suite: static parseInt/parseFloat, formatting, conversion, valueOf!)
- **Object Methods**: 9+ (ES5, ES2015, ES2017 & ES2022 features - complete introspection + manipulation + immutability!)
- **Global Functions**: 4+ (NaN & Finite detection, parseInt & parseFloat parsing!)
- **Console Methods**: 14+ (logging + console control + performance timing + assertions + counters + tables + grouping: log, error, warn, info, debug, clear, time, timeEnd, assert, count, countReset, table, group, groupEnd!)

