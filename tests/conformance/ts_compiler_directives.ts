// NOVA_TEST_MODE: check
// NOVA_EXPECT_EXIT: 0
//
// Local regression gate for Phase 2 Step 1: the `// @directive` compiler-option
// parser must be read from the leading comments and plumbed into the
// TypeChecker without changing checker behaviour (Step 1 is scaffolding only;
// the flags are not yet consumed by checker logic). This file uses several
// directives (including `// @strict`) and must still type-check successfully
// because the strict features are not yet active.

// @strict
// @strictNullChecks
// @noImplicitAny
// @noUnusedLocals

// A construct that *would* error under strictNullChecks once Phase 2 Step 2
// activates the flag, but is currently accepted (Step 1 only plumbs options).
let maybeNull: string | null = null;
let len: number = maybeNull ? maybeNull.length : 0;

function greet(name: string): string {
    return "hi " + name;
}

const result: string = greet("world");
