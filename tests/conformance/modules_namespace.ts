// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

import * as math from "../fixtures/modules/math";

function main(): number {
    if (!(math.add(math.base, 2) === 42)) return 1;
    if (!(math.label === "nova")) return 2;
    if (!(math.enabled === true)) return 3;
    return 0;
}
