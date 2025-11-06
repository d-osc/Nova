// Test basic string operations
function testStringBasics(): number {
    let str: string = "Hello, Nova!";
    let empty: string = "";
    let num: number = 0;
    
    if (str) {
        num = 1;
    } else {
        num = 0;
    }
    
    return num;
}

function testStringConcat(): number {
    let str1: string = "Hello";
    let str2: string = ", ";
    let str3: string = "Nova";
    let result: string = str1 + str2 + str3;
    let num: number = 0;
    
    if (result == "Hello, Nova") {
        num = 1;
    } else {
        num = 0;
    }
    
    return num;
}

function main(): number {
    let result1 = testStringBasics();
    let result2 = testStringConcat();
    return result1 + result2;
}