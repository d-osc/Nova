// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    function tag(strings: string[], ...values: number[]): string {
        let result = strings[0];
        for (let index = 0; index < values.length; index++) {
            result += values[index] * 2;
            result += strings[index + 1];
        }
        return result;
    }
    return tag`a=${10}b=${20}c` === "a=20b=40c" ? 0 : 1;
}
