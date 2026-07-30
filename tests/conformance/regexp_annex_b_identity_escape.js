// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main() {
    function check(value, code) {
        if (!value) return code;
        return 0;
    }
    let result = check(/\x/.test("x"), 1);
    if (result) return result;
    result = check(/\xa/.test("xa"), 2);
    if (result) return result;
    result = check(/\u/.test("u"), 3);
    if (result) return result;
    result = check(/\ua/.test("ua"), 4);
    if (result) return result;
    return 0;
}
