// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    let plusOne = function(value) {
        return value + 1;
    };
    let identity = (value) => value;

    if (!(plusOne(4) === 5)) return 1;
    if (!(plusOne("4") === "41")) return 2;
    if (!(identity(9) === 9)) return 3;
    if (!(identity("nine") === "nine")) return 4;
    return 0;
}
