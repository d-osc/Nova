// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    let date = new Date(0);
    let result: any = date.setYear();
    if (result === result) return 1;
    result = date.valueOf();
    if (result === result) return 2;

    date = new Date(0);
    result = date.setYear(NaN);
    if (result === result) return 3;
    result = date.valueOf();
    if (result === result) return 4;

    date = new Date(1970, 0);
    date.setYear(-1);
    if (date.getFullYear() !== -1) return 5;

    date = new Date(1970, 0);
    date.setYear(50.999999);
    if (date.getFullYear() !== 1950) return 6;

    date = new Date(1970, 8, 10, 0, 0, 0, 0);
    result = date.setYear(275760);
    if (result !== result) return 7;

    date = new Date(1970, 8, 14, 0, 0, 0, 0);
    result = date.setYear(275760);
    if (result === result) return 8;
    result = date.valueOf();
    if (result === result) return 9;

    return 0;
}
