// Test basic array operations
function testArrayCreation(): number {
    let arr: number[] = [1, 2, 3, 4, 5];
    let num: number = 0;
    
    if (arr.length > 0) {
        num = 1;
    } else {
        num = 0;
    }
    
    return num;
}

function testArrayAccess(): number {
    let arr: number[] = [10, 20, 30];
    let sum: number = arr[0] + arr[1] + arr[2];
    return sum;
}

function testArrayModification(): number {
    let arr: number[] = [1, 2, 3];
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    
    return arr[0] + arr[1] + arr[2];
}

function main(): number {
    let result1 = testArrayCreation();
    let result2 = testArrayAccess();
    let result3 = testArrayModification();
    return result1 + result2 + result3;
}