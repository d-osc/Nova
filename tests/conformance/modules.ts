// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

import defaultValue, { add as sum, base, label, enabled } from "../fixtures/modules/math";

function main(): number {
    if (!(sum(base, 2) === 42)) return 1;
    if (!(label === "nova")) return 2;
    if (!(enabled === true)) return 3;
    if (!(defaultValue === 7)) return 4;
    return 0;
}
