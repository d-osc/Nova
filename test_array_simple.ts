// Test simple array operations with function calls

function testArrayCreation(): number {
    // For now, we'll use function calls to create arrays
    let arr = createArray(3);
    setArrayElement(arr, 0, 1);
    setArrayElement(arr, 1, 2);
    setArrayElement(arr, 2, 3);
    
    return getArrayElement(arr, 0);
}

function testArrayAccess(): number {
    let arr = createArray(3);
    setArrayElement(arr, 0, 10);
    setArrayElement(arr, 1, 20);
    setArrayElement(arr, 2, 30);
    
    let sum = getArrayElement(arr, 0) + getArrayElement(arr, 1) + getArrayElement(arr, 2);
    return sum;
}

function main(): number {
    let result1 = testArrayCreation();
    let result2 = testArrayAccess();
    return result1 + result2;
}

// These would be built-in functions or runtime functions
function createArray(size) {
    return size; // Placeholder
}

function setArrayElement(arr, index, value) {
    // Placeholder
    return 0; // Return a value instead of void
}

function getArrayElement(arr, index) {
    return 0; // Placeholder
}