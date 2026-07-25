// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    let object = { first: 10, second: "Nova" };

    let first = Object.getOwnPropertyDescriptor(object, "first");
    if (first.value != 10) return 1;
    if (!first.writable) return 2;
    if (!first.enumerable) return 3;
    if (!first.configurable) return 4;

    let missing = Object.getOwnPropertyDescriptor(object, "missing");
    if (missing != undefined) return 5;

    let descriptors = Object.getOwnPropertyDescriptors(object);
    if (descriptors.first.value != 10) return 6;
    if (descriptors.second.value != "Nova") return 7;
    if (!descriptors.second.writable) return 8;

    Object.freeze(object);
    let frozen = Object.getOwnPropertyDescriptor(object, "first");
    if (frozen.writable) return 9;
    if (frozen.configurable) return 10;
    if (!frozen.enumerable) return 11;

    let sealedObject = { value: 20 };
    Object.seal(sealedObject);
    let sealed = Object.getOwnPropertyDescriptor(sealedObject, "value");
    if (!sealed.writable) return 12;
    if (sealed.configurable) return 13;

    return 0;
}
