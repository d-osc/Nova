// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    let object = { value: 1 };
    let returned = Object.defineProperty(object, "value", {
        value: 2,
        writable: false,
        enumerable: false,
        configurable: false
    });

    if (!Object.is(object, returned)) return 1;
    if (object.value != 2) return 2;

    object.value = 3;
    if (object.value != 2) return 3;
    returned.value = 4;
    if (returned.value != 2) return 4;

    let descriptor = Object.getOwnPropertyDescriptor(object, "value");
    if (descriptor.value != 2) return 5;
    if (descriptor.writable) return 6;
    if (descriptor.enumerable) return 7;
    if (descriptor.configurable) return 8;

    if (Object.keys(object).length != 0) return 9;
    if (Object.values(object).length != 0) return 10;
    if (Object.entries(object).length != 0) return 11;
    if (Object.getOwnPropertyNames(object).length != 1) return 12;

    Object.defineProperty(object, "value", {
        value: 9,
        writable: true,
        enumerable: true,
        configurable: true
    });
    if (object.value != 2) return 13;
    if (Object.keys(object).length != 0) return 14;

    return 0;
}
