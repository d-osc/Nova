// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0
// NOVA_EXPECT_STDOUT_CONTAINS: "0 false null undefined x 2.5\n"

function main(): number {
    let object = { value: 7 };
    let values = [0, false, null, undefined, "", "x", 2.5, object];

    if (!(values[0] === 0)) return 1;
    if (values[0] === false) return 2;
    if (!(values[1] === false)) return 3;
    if (values[1] === 0) return 4;

    if (!(values[2] === null)) return 5;
    if (values[2] === undefined) return 6;
    if (!(values[2] == undefined)) return 7;
    if (!(values[3] === undefined)) return 8;
    if (values[3] === null) return 9;

    if (values[4]) return 10;
    if (!values[5]) return 11;
    if (!(values[6] === 2.5)) return 12;
    if (!(values[7] === object)) return 13;

    if (!((values[2] ?? 41) === 41)) return 14;
    if (!((values[3] ?? 42) === 42)) return 15;
    if (!((values[0] ?? 43) === 0)) return 16;

    values[0] = true;
    values[1] = 9.5;
    values[2] = "changed";
    values[3] = object;
    if (!(values[0] === true)) return 17;
    if (!(values[1] === 9.5)) return 18;
    if (!(values[2] === "changed")) return 19;
    if (!(values[3] === object)) return 20;

    values[0] = 0;
    values[1] = false;
    values[2] = null;
    values[3] = undefined;

    if (values.push("tail") != 9) return 21;
    if (!(values.pop() === "tail")) return 22;
    if (values.unshift(undefined) != 9) return 23;
    if (!(values.shift() === undefined)) return 24;
    if (!(values.at(-1) === object)) return 25;
    if (!values.includes(false)) return 26;
    if (values.includes(true)) return 27;
    if (values.indexOf(null) != 2) return 28;
    if (values.lastIndexOf(undefined) != 3) return 29;

    let sliced = values.slice(1, 4);
    if (!(sliced[0] === false)) return 30;
    if (!(sliced[1] === null)) return 31;
    if (!(sliced[2] === undefined)) return 32;
    let concatenated = sliced.concat(sliced);
    if (!(concatenated[3] === false)) return 33;
    if (!(concatenated[4] === null)) return 34;
    if (!(concatenated[5] === undefined)) return 35;

    let dynamicObject = { value: 1, stable: 5 };
    if (dynamicObject.stable + 2 != 7) return 36;
    dynamicObject.value = "text";
    if (!(dynamicObject.value === "text")) return 37;
    dynamicObject.value = false;
    if (!(dynamicObject.value === false)) return 38;
    dynamicObject.value = null;
    if (!(dynamicObject.value === null)) return 39;
    dynamicObject.value = undefined;
    if (!(dynamicObject.value === undefined)) return 40;
    dynamicObject.value = 2.5;
    if (!(dynamicObject.value === 2.5)) return 41;
    dynamicObject.value = object;
    if (!(dynamicObject.value === object)) return 42;

    let dynamicBinding = 1;
    if (!(dynamicBinding === 1)) return 43;
    dynamicBinding = "binding";
    if (!(dynamicBinding === "binding")) return 44;
    dynamicBinding = false;
    if (!(dynamicBinding === false)) return 45;
    if (dynamicBinding) return 46;
    dynamicBinding = null;
    if (!(dynamicBinding === null)) return 47;
    if (!((dynamicBinding ?? 51) === 51)) return 48;
    dynamicBinding = undefined;
    if (!(dynamicBinding === undefined)) return 49;
    dynamicBinding = 3.25;
    if (!(dynamicBinding === 3.25)) return 50;
    dynamicBinding = object;
    if (!(dynamicBinding === object)) return 51;
    dynamicBinding = null;
    dynamicBinding ??= "assigned";
    if (!(dynamicBinding === "assigned")) return 52;
    dynamicBinding &&= 0;
    if (!(dynamicBinding === 0)) return 53;
    dynamicBinding ||= true;
    if (!(dynamicBinding === true)) return 54;
    let uninitialized;
    if (!(uninitialized === undefined)) return 55;
    if (uninitialized === null) return 56;
    uninitialized ??= 12;
    if (!(uninitialized === 12)) return 57;

    console.log(values[0], values[1], values[2], values[3], values[5], values[6]);
    return 0;
}
