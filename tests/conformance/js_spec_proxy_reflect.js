// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main() {
    const log = [];
    const target = { value: 2 };
    const proxy = new Proxy(target, {
        get(current, key, receiver) {
            log.push("get:" + String(key));
            return Reflect.get(current, key, receiver);
        },
        set(current, key, value, receiver) {
            log.push("set:" + String(key));
            return Reflect.set(current, key, value * 2, receiver);
        },
        has(current, key) {
            log.push("has:" + String(key));
            return Reflect.has(current, key);
        },
        deleteProperty(current, key) {
            log.push("delete:" + String(key));
            return Reflect.deleteProperty(current, key);
        }
    });

    if (proxy.value !== 2) return 1;
    proxy.value = 5;
    if (target.value !== 10) return 2;
    if (!("value" in proxy)) return 3;
    if (!delete proxy.value) return 4;
    if ("value" in target) return 5;
    if (log.join("|") !== "get:value|set:value|has:value|delete:value") return 6;

    function add(left, right) { return this.base + left + right; }
    if (Reflect.apply(add, { base: 1 }, [2, 3]) !== 6) return 7;

    const revocable = Proxy.revocable({ ok: true }, {});
    if (!revocable.proxy.ok) return 8;
    revocable.revoke();
    let revoked = false;
    try {
        revocable.proxy.ok;
    } catch (error) {
        revoked = error instanceof TypeError;
    }
    if (!revoked) return 9;

    return 0;
}
