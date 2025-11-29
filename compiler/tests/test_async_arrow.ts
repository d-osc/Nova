// Test async arrow function

function main(): number {
    let asyncArrow = async (x: number): number => {
        return x * 2;
    };

    let result: number = asyncArrow(21);
    if (result != 42) {
        return 1;
    }
    return 0;
}
