# Nova Compiler Changelog

## v0.77.0 - Array.slice() method
- Added Array.slice(start, end) method
- Creates sub-array from start to end index
- Supports negative indices
- Returns new array without modifying original

## v0.76.0 - Array.concat() method  
- Added Array.concat(otherArray) method
- Concatenates two arrays into new array
- Original arrays remain unchanged
- Returns metadata-compatible structure

## v0.75.0 - Array.join() method
- Added Array.join(delimiter) method
- Joins array elements into string
- Customizable delimiter (default comma)
- Includes concat and slice runtime implementations

## v0.74.0 - String.split() method
- Added String.split(delimiter) method
- Splits string into string array
- New StringArray structure added
- Handles empty delimiters and edge cases

## v0.73.0 - String.padEnd() method
- Added String.padEnd() method

## v0.72.0 - String.padStart() method
- Added String.padStart() method
