// Combined test: includes() and indexOf()
function main(): number {
    let arr = [5, 15, 25, 35];

    // Use indexOf to find position
    let pos = arr.indexOf(25);  // pos = 2

    // Use includes to check existence
    let hasValue = arr.includes(15);  // hasValue = 1

    // Get element at found position
    let elem = arr[pos];  // elem = 25

    // Return: 2 + 1 + 25 = 28
    return pos + hasValue + elem;
}
