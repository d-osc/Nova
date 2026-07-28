// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

class A extends Error {}
class B extends A {}
class C extends B {}

function main(): number {
    const error = new C("test");
    if (!(error instanceof C)) return 1;
    if (!(error instanceof B)) return 2;
    if (!(error instanceof A)) return 3;
    if (!(error instanceof Error)) return 4;
    return 0;
}
