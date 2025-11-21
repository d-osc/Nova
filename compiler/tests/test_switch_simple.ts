// Test basic switch/case statement
function main(): number {
    let x = 2;
    let result = 0;
    
    switch (x) {
        case 1:
            result = 10;
            break;
        case 2:
            result = 20;
            break;
        case 3:
            result = 30;
            break;
        default:
            result = 99;
            break;
    }
    
    return result;  // Should return 20
}
