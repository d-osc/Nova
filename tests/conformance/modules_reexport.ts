// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

import { plus, base, label } from "../fixtures/modules/reexport";

function main(): number {
    if (!(plus(base, 2) === 42)) return 1;
    if (!(label === "nova")) return 2;
    return 0;
}
