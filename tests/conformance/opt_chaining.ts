// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    // Probe what optional chaining actually returns
    const nothing: any = null;
    const val = nothing?.x;
    console.log("val =", val, "(expected undefined/null)");

    // Try a real null deref that should crash without short-circuit
    const crash: any = null;
    const shouldNotCrash = crash?.x.y.z;
    console.log("shouldNotCrash =", shouldNotCrash);

    return 0;
}
