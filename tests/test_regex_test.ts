// Test regex.test() method
function main(): number {
    let regex = /hello/;
    let str = "hello world";

    if (regex.test(str)) {
        console.log("Match found!");
        return 1;
    } else {
        console.log("No match");
        return 0;
    }
}
