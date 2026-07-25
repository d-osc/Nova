// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    let large = 123456789012345678901234567890n;
    let one = 1n;
    let sum = large + one;
    if (!(sum.toString() === "123456789012345678901234567891")) return 1;

    let product = 12n * 11n;
    if (!(product.toString() === "132")) return 2;

    let quotient = 100n / 9n;
    let remainder = 100n % 9n;
    if (!(quotient.toString() === "11")) return 3;
    if (!(remainder.toString() === "1")) return 4;

    let power = 2n ** 10n;
    if (!(power.toString() === "1024")) return 5;
    if (!(1n < 2n)) return 6;
    if (!(2n >= 2n)) return 7;
    if (!(2n === BigInt(2))) return 8;
    if (!(2n !== 3n)) return 9;

    let wrapped = BigInt.asUintN(8, BigInt(300));
    if (!(wrapped.toString() === "44")) return 10;

    let negative = -5n;
    if (!(negative.toString() === "-5")) return 11;
    let inverted = ~5n;
    if (!(inverted === -6n)) return 12;
    if (!(inverted.toString() === "-6")) return 23;
    if (!(typeof negative === "bigint")) return 13;

    let shiftedLeft = 3n << 4n;
    let shiftedRight = 48n >> 4n;
    if (!(shiftedLeft.toString() === "48")) return 14;
    if (!(shiftedRight.toString() === "3")) return 15;

    let updated = 4n;
    if (!((updated++) === 4n)) return 16;
    if (!((++updated) === 6n)) return 17;
    if (!((updated--) === 6n)) return 18;
    if (!((--updated) === 4n)) return 19;

    let compound = 10n;
    compound += 5n;
    compound *= 3n;
    compound -= 4n;
    compound /= 2n;
    compound %= 6n;
    compound **= 3n;
    if (!(compound.toString() === "8")) return 20;
    compound = 1n;
    compound |= 6n;
    compound &= 3n;
    compound ^= 2n;
    if (!(compound.toString() === "1")) return 21;
    compound <<= 4n;
    compound >>= 2n;
    if (!(compound.toString() === "4")) return 22;
    return 0;
}
