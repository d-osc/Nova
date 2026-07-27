// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main() {
    const proto = { inherited: 3 };
    const object = Object.create(proto);
    Object.defineProperty(object, "hidden", {
        value: 7,
        writable: false,
        enumerable: false,
        configurable: true
    });
    object.visible = 9;

    if (object.inherited !== 3) return 1;
    if (!proto.isPrototypeOf(object)) return 2;
    if (Object.getPrototypeOf(object) !== proto) return 3;
    if (Object.keys(object).join(",") !== "visible") return 4;
    if (Object.getOwnPropertyNames(object).join(",") !== "hidden,visible") return 5;

    const descriptor = Object.getOwnPropertyDescriptor(object, "hidden");
    if (!descriptor || descriptor.value !== 7) return 6;
    if (descriptor.writable || descriptor.enumerable || !descriptor.configurable) return 7;

    object.hidden = 10;
    if (object.hidden !== 7) return 8;
    if (!delete object.hidden) return 9;
    if ("hidden" in object) return 10;

    const symbol = Symbol("secret");
    object[symbol] = 11;
    const ownKeys = Reflect.ownKeys(object);
    if (ownKeys.length !== 2) return 11;
    if (ownKeys[0] !== "visible" || ownKeys[1] !== symbol) return 12;

    Object.preventExtensions(object);
    if (Object.isExtensible(object)) return 13;
    object.extra = 1;
    if ("extra" in object) return 14;

    return 0;
}
