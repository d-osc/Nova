// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    if (!(1 === 1.0)) return 1;
    if (1 === true) return 2;
    if (0 === false) return 3;
    if ("1" === 1) return 4;
    if (!("nova" === "nova")) return 5;
    if (!(null === null)) return 6;
    if (!(undefined === undefined)) return 7;
    if (null === undefined) return 8;
    if (!(null == undefined)) return 9;
    if (null != undefined) return 10;
    if (!(null !== undefined)) return 11;
    if (NaN === NaN) return 12;
    if (!(-0.0 === 0.0)) return 13;

    let object = { value: 1 };
    let alias = object;
    let other = { value: 1 };
    if (!(object === alias)) return 14;
    if (object === other) return 15;

    let array = [1, 2];
    let arrayAlias = array;
    let otherArray = [1, 2];
    if (!(array === arrayAlias)) return 16;
    if (array === otherArray) return 17;

    if (!("1" == 1)) return 18;
    if (!(1 == "1")) return 19;
    if (!("  12.5  " == 12.5)) return 20;
    if (!("" == 0)) return 21;
    if (!("   " == 0)) return 22;
    if ("nova" == 0) return 23;
    if (!(true == 1)) return 24;
    if (!(false == 0)) return 25;
    if (!("1" == true)) return 26;
    if ("0" == true) return 27;
    if (object == other) return 28;
    if (!(object == alias)) return 29;

    return 0;
}
