// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    let frozen = { value: 1 };
    let frozenAlias = Object.freeze(frozen);
    if (!Object.isFrozen(frozen)) return 1;
    if (!Object.isFrozen(frozenAlias)) return 2;
    if (!Object.isSealed(frozen)) return 3;
    if (Object.isExtensible(frozen)) return 4;
    frozen.value = 99;
    frozenAlias.value = 88;
    if (frozen.value != 1) return 5;
    Object.assign(frozen, { value: 77 });
    if (frozen.value != 1) return 14;

    let sealed = { value: 2 };
    Object.seal(sealed);
    if (!Object.isSealed(sealed)) return 6;
    if (Object.isFrozen(sealed)) return 7;
    if (Object.isExtensible(sealed)) return 8;
    sealed.value = 20;
    if (sealed.value != 20) return 9;

    let fixed = { value: 3 };
    let fixedAlias = Object.preventExtensions(fixed);
    if (Object.isExtensible(fixed)) return 10;
    if (Object.isExtensible(fixedAlias)) return 11;
    if (Object.isSealed(fixed)) return 12;
    fixed.value = 30;
    if (fixed.value != 30) return 13;
    return 0;
}
