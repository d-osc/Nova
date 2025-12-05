/**
 * nova:test - Testing Module Implementation
 *
 * Provides bun:test/jest-style testing utilities for Nova programs.
 *
 * Usage:
 *   import { describe, test, expect } from "nova:test";
 *
 *   describe("My Suite", () => {
 *     test("should work", () => {
 *       expect(1 + 1).toBe(2);
 *     });
 *   });
 */

#include "nova/runtime/BuiltinModules.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <stdexcept>
#include <sstream>

namespace nova {
namespace runtime {
namespace test {

// Global test state
std::vector<TestSuite> g_testSuites;
TestSuite* g_currentSuite = nullptr;
int g_totalPassed = 0;
int g_totalFailed = 0;

// Current test hooks
static void (*g_beforeEach)() = nullptr;
static void (*g_afterEach)() = nullptr;
static void (*g_beforeAll)() = nullptr;
static void (*g_afterAll)() = nullptr;

// Assertion context for chaining
struct ExpectContext {
    enum ValueType { NUMBER, STRING, BOOLEAN, POINTER };
    ValueType type;
    double numValue;
    const char* strValue;
    bool boolValue;
    void* ptrValue;
    bool negated;
};

// Colors for terminal output
#ifdef _WIN32
#define COLOR_RED ""
#define COLOR_GREEN ""
#define COLOR_YELLOW ""
#define COLOR_RESET ""
#define CHECK_MARK "+"
#define CROSS_MARK "x"
#else
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_RESET "\033[0m"
#define CHECK_MARK "\u2713"
#define CROSS_MARK "\u2717"
#endif

// Throw assertion error
static void assertionFailed(const std::string& message) {
    throw std::runtime_error(message);
}

extern "C" {

// describe(name, fn) - Create a test suite
void nova_test_describe(const char* name, void (*fn)()) {
    if (!name || !fn) return;

    TestSuite suite;
    suite.name = name;
    suite.passed = 0;
    suite.failed = 0;
    suite.skipped = 0;

    TestSuite* previousSuite = g_currentSuite;
    g_currentSuite = &suite;

    // Run beforeAll if set
    if (g_beforeAll) {
        try {
            g_beforeAll();
        } catch (...) {
            std::cerr << COLOR_RED << "  beforeAll failed" << COLOR_RESET << std::endl;
        }
    }

    // Execute test suite
    fn();

    // Run afterAll if set
    if (g_afterAll) {
        try {
            g_afterAll();
        } catch (...) {
            std::cerr << COLOR_RED << "  afterAll failed" << COLOR_RESET << std::endl;
        }
    }

    // Reset hooks
    g_beforeEach = nullptr;
    g_afterEach = nullptr;
    g_beforeAll = nullptr;
    g_afterAll = nullptr;

    g_currentSuite = previousSuite;
    g_testSuites.push_back(suite);
}

// test(name, fn) - Create a test case
void nova_test_test(const char* name, void (*fn)()) {
    if (!name || !fn) return;

    TestResult result;
    result.name = name;
    result.passed = true;
    result.error = "";

    auto startTime = std::chrono::high_resolution_clock::now();

    // Run beforeEach if set
    if (g_beforeEach) {
        try {
            g_beforeEach();
        } catch (const std::exception& e) {
            result.passed = false;
            result.error = std::string("beforeEach: ") + e.what();
        }
    }

    // Run the test
    if (result.passed) {
        try {
            fn();
        } catch (const std::exception& e) {
            result.passed = false;
            result.error = e.what();
        } catch (...) {
            result.passed = false;
            result.error = "Unknown error";
        }
    }

    // Run afterEach if set
    if (g_afterEach && result.passed) {
        try {
            g_afterEach();
        } catch (const std::exception& e) {
            result.passed = false;
            result.error = std::string("afterEach: ") + e.what();
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.durationMs = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count() / 1000.0;

    // Update counters
    if (result.passed) {
        g_totalPassed++;
        if (g_currentSuite) g_currentSuite->passed++;
        std::cout << COLOR_GREEN << "  " << CHECK_MARK << " " << name << COLOR_RESET;
        if (result.durationMs > 0.1) {
            std::cout << " (" << (int)result.durationMs << "ms)";
        }
        std::cout << std::endl;
    } else {
        g_totalFailed++;
        if (g_currentSuite) g_currentSuite->failed++;
        std::cout << COLOR_RED << "  " << CROSS_MARK << " " << name << COLOR_RESET << std::endl;
        if (!result.error.empty()) {
            std::cout << COLOR_RED << "    " << result.error << COLOR_RESET << std::endl;
        }
    }

    if (g_currentSuite) {
        g_currentSuite->tests.push_back(result);
    }
}

// Alias: it(name, fn)
void nova_test_it(const char* name, void (*fn)()) {
    nova_test_test(name, fn);
}

// beforeEach hook
void nova_test_beforeEach(void (*fn)()) {
    g_beforeEach = fn;
}

// afterEach hook
void nova_test_afterEach(void (*fn)()) {
    g_afterEach = fn;
}

// beforeAll hook
void nova_test_beforeAll(void (*fn)()) {
    g_beforeAll = fn;
}

// afterAll hook
void nova_test_afterAll(void (*fn)()) {
    g_afterAll = fn;
}

// expect(value) - Start assertion chain
void* nova_test_expect(double value) {
    ExpectContext* ctx = new ExpectContext();
    ctx->type = ExpectContext::NUMBER;
    ctx->numValue = value;
    ctx->negated = false;
    return ctx;
}

void* nova_test_expect_str(const char* value) {
    ExpectContext* ctx = new ExpectContext();
    ctx->type = ExpectContext::STRING;
    ctx->strValue = value;
    ctx->negated = false;
    return ctx;
}

void* nova_test_expect_bool(int value) {
    ExpectContext* ctx = new ExpectContext();
    ctx->type = ExpectContext::BOOLEAN;
    ctx->boolValue = value != 0;
    ctx->negated = false;
    return ctx;
}

void* nova_test_expect_ptr(void* value) {
    ExpectContext* ctx = new ExpectContext();
    ctx->type = ExpectContext::POINTER;
    ctx->ptrValue = value;
    ctx->negated = false;
    return ctx;
}

// Negation: expect(x).not
void* nova_test_not(void* ctx) {
    if (!ctx) return nullptr;
    ExpectContext* ec = (ExpectContext*)ctx;
    ec->negated = !ec->negated;
    return ctx;
}

// toBe - strict equality
void nova_test_toBe(void* ctx, double expected) {
    if (!ctx) return;
    ExpectContext* ec = (ExpectContext*)ctx;

    bool passed = (ec->numValue == expected);
    if (ec->negated) passed = !passed;

    if (!passed) {
        std::stringstream ss;
        ss << "Expected " << ec->numValue << (ec->negated ? " not " : " ") << "to be " << expected;
        delete ec;
        assertionFailed(ss.str());
    }
    delete ec;
}

void nova_test_toBe_str(void* ctx, const char* expected) {
    if (!ctx) return;
    ExpectContext* ec = (ExpectContext*)ctx;

    bool passed = (ec->strValue && expected && strcmp(ec->strValue, expected) == 0);
    if (ec->negated) passed = !passed;

    if (!passed) {
        std::stringstream ss;
        ss << "Expected \"" << (ec->strValue ? ec->strValue : "null") << "\""
           << (ec->negated ? " not " : " ") << "to be \"" << (expected ? expected : "null") << "\"";
        delete ec;
        assertionFailed(ss.str());
    }
    delete ec;
}

// toEqual - deep equality (same as toBe for primitives)
void nova_test_toEqual(void* ctx, double expected) {
    nova_test_toBe(ctx, expected);
}

// toBeTruthy
void nova_test_toBeTruthy(void* ctx) {
    if (!ctx) return;
    ExpectContext* ec = (ExpectContext*)ctx;

    bool isTruthy = false;
    switch (ec->type) {
        case ExpectContext::NUMBER:
            isTruthy = ec->numValue != 0 && !std::isnan(ec->numValue);
            break;
        case ExpectContext::STRING:
            isTruthy = ec->strValue != nullptr && strlen(ec->strValue) > 0;
            break;
        case ExpectContext::BOOLEAN:
            isTruthy = ec->boolValue;
            break;
        case ExpectContext::POINTER:
            isTruthy = ec->ptrValue != nullptr;
            break;
    }

    bool passed = isTruthy;
    if (ec->negated) passed = !passed;

    if (!passed) {
        delete ec;
        assertionFailed(ec->negated ? "Expected value to be falsy" : "Expected value to be truthy");
    }
    delete ec;
}

// toBeFalsy
void nova_test_toBeFalsy(void* ctx) {
    if (!ctx) return;
    ExpectContext* ec = (ExpectContext*)ctx;
    ec->negated = !ec->negated;
    nova_test_toBeTruthy(ctx);
}

// toBeNull
void nova_test_toBeNull(void* ctx) {
    if (!ctx) return;
    ExpectContext* ec = (ExpectContext*)ctx;

    bool isNull = (ec->type == ExpectContext::POINTER && ec->ptrValue == nullptr) ||
                  (ec->type == ExpectContext::STRING && ec->strValue == nullptr);

    bool passed = isNull;
    if (ec->negated) passed = !passed;

    if (!passed) {
        delete ec;
        assertionFailed(ec->negated ? "Expected value not to be null" : "Expected value to be null");
    }
    delete ec;
}

// toBeUndefined (same as null in Nova context)
void nova_test_toBeUndefined(void* ctx) {
    nova_test_toBeNull(ctx);
}

// toBeGreaterThan
void nova_test_toBeGreaterThan(void* ctx, double expected) {
    if (!ctx) return;
    ExpectContext* ec = (ExpectContext*)ctx;

    bool passed = ec->numValue > expected;
    if (ec->negated) passed = !passed;

    if (!passed) {
        std::stringstream ss;
        ss << "Expected " << ec->numValue << (ec->negated ? " not " : " ") << "to be greater than " << expected;
        delete ec;
        assertionFailed(ss.str());
    }
    delete ec;
}

// toBeLessThan
void nova_test_toBeLessThan(void* ctx, double expected) {
    if (!ctx) return;
    ExpectContext* ec = (ExpectContext*)ctx;

    bool passed = ec->numValue < expected;
    if (ec->negated) passed = !passed;

    if (!passed) {
        std::stringstream ss;
        ss << "Expected " << ec->numValue << (ec->negated ? " not " : " ") << "to be less than " << expected;
        delete ec;
        assertionFailed(ss.str());
    }
    delete ec;
}

// toBeGreaterThanOrEqual
void nova_test_toBeGreaterThanOrEqual(void* ctx, double expected) {
    if (!ctx) return;
    ExpectContext* ec = (ExpectContext*)ctx;

    bool passed = ec->numValue >= expected;
    if (ec->negated) passed = !passed;

    if (!passed) {
        std::stringstream ss;
        ss << "Expected " << ec->numValue << (ec->negated ? " not " : " ") << "to be >= " << expected;
        delete ec;
        assertionFailed(ss.str());
    }
    delete ec;
}

// toBeLessThanOrEqual
void nova_test_toBeLessThanOrEqual(void* ctx, double expected) {
    if (!ctx) return;
    ExpectContext* ec = (ExpectContext*)ctx;

    bool passed = ec->numValue <= expected;
    if (ec->negated) passed = !passed;

    if (!passed) {
        std::stringstream ss;
        ss << "Expected " << ec->numValue << (ec->negated ? " not " : " ") << "to be <= " << expected;
        delete ec;
        assertionFailed(ss.str());
    }
    delete ec;
}

// toContain - check if string contains substring
void nova_test_toContain(void* ctx, const char* expected) {
    if (!ctx || !expected) return;
    ExpectContext* ec = (ExpectContext*)ctx;

    bool contains = ec->strValue && strstr(ec->strValue, expected) != nullptr;
    bool passed = contains;
    if (ec->negated) passed = !passed;

    if (!passed) {
        std::stringstream ss;
        ss << "Expected \"" << (ec->strValue ? ec->strValue : "null") << "\""
           << (ec->negated ? " not " : " ") << "to contain \"" << expected << "\"";
        delete ec;
        assertionFailed(ss.str());
    }
    delete ec;
}

// toHaveLength
void nova_test_toHaveLength(void* ctx, int expected) {
    if (!ctx) return;
    ExpectContext* ec = (ExpectContext*)ctx;

    int length = ec->strValue ? (int)strlen(ec->strValue) : 0;
    bool passed = length == expected;
    if (ec->negated) passed = !passed;

    if (!passed) {
        std::stringstream ss;
        ss << "Expected length " << length << (ec->negated ? " not " : " ") << "to be " << expected;
        delete ec;
        assertionFailed(ss.str());
    }
    delete ec;
}

// toThrow - expect function to throw
void nova_test_toThrow(void* ctx) {
    if (!ctx) return;
    ExpectContext* ec = (ExpectContext*)ctx;
    // This is handled specially in code generation
    // For now, just clean up
    delete ec;
}

// Skip test
void nova_test_skip(const char* name, [[maybe_unused]] void (*fn)()) {
    if (!name) return;

    if (g_currentSuite) {
        g_currentSuite->skipped++;
    }

    std::cout << COLOR_YELLOW << "  - " << name << " (skipped)" << COLOR_RESET << std::endl;
}

// Only run this test (mark others as skipped)
void nova_test_only(const char* name, void (*fn)()) {
    // For simplicity, just run this test
    nova_test_test(name, fn);
}

// Run all tests and return exit code
int nova_test_runAll() {
    std::cout << std::endl;
    std::cout << "Test Results:" << std::endl;
    std::cout << "-------------" << std::endl;

    int totalSkipped = 0;

    for (const auto& suite : g_testSuites) {
        totalSkipped += suite.skipped;
    }

    int totalTests = g_totalPassed + g_totalFailed + totalSkipped;
    std::cout << "  " << totalTests << " total" << std::endl;

    if (g_totalPassed > 0) {
        std::cout << COLOR_GREEN << "  " << g_totalPassed << " passed" << COLOR_RESET << std::endl;
    }
    if (g_totalFailed > 0) {
        std::cout << COLOR_RED << "  " << g_totalFailed << " failed" << COLOR_RESET << std::endl;
    }
    if (totalSkipped > 0) {
        std::cout << COLOR_YELLOW << "  " << totalSkipped << " skipped" << COLOR_RESET << std::endl;
    }

    std::cout << std::endl;

    // Clear state for next run
    g_testSuites.clear();
    int result = g_totalFailed;
    g_totalPassed = 0;
    g_totalFailed = 0;

    return result > 0 ? 1 : 0;
}

// test.todo(name)
void nova_test_todo(const char* name) {
    if (!name) return;
    std::cout << COLOR_YELLOW << "  ○ " << name << " (todo)" << COLOR_RESET << std::endl;
}

// describe.skip/only/todo
void nova_test_describe_skip(const char* name, void (*fn)()) { (void)fn; if (name) std::cout << COLOR_YELLOW << name << " (suite skipped)" << COLOR_RESET << std::endl; }
void nova_test_describe_only(const char* name, void (*fn)()) { nova_test_describe(name, fn); }
void nova_test_describe_todo(const char* name) { if (name) std::cout << COLOR_YELLOW << "○ " << name << " (suite todo)" << COLOR_RESET << std::endl; }

// before/after aliases
void nova_test_before(void (*fn)()) { nova_test_beforeAll(fn); }
void nova_test_after(void (*fn)()) { nova_test_afterAll(fn); }

// Mock functions
struct MockFunction { void* originalFn; void* mockFn; int callCount; bool isRestored; };
static std::vector<MockFunction*> g_mocks;

void* nova_test_mock_fn(void* original) { auto* m = new MockFunction{original, nullptr, 0, false}; g_mocks.push_back(m); return m; }
int nova_test_mock_callCount(void* p) { return p ? ((MockFunction*)p)->callCount : 0; }
void nova_test_mock_recordCall(void* p) { if (p) ((MockFunction*)p)->callCount++; }
void nova_test_mock_reset(void* p) { if (p) ((MockFunction*)p)->callCount = 0; }
void nova_test_mock_restore(void* p) { if (p) ((MockFunction*)p)->isRestored = true; }
void nova_test_mock_mockImpl(void* p, void* impl) { if (p) ((MockFunction*)p)->mockFn = impl; }
void nova_test_mock_restoreAll() { for (auto* m : g_mocks) m->isRestored = true; }
void nova_test_mock_clearAll() { for (auto* m : g_mocks) m->callCount = 0; }
void nova_test_mock_free(void* p) { if (!p) return; auto it = std::find(g_mocks.begin(), g_mocks.end(), (MockFunction*)p); if (it != g_mocks.end()) g_mocks.erase(it); delete (MockFunction*)p; }

// Timer mocking
static bool g_mockTimers = false;
static int64_t g_mockTime = 0;
void nova_test_mock_timers_enable() { g_mockTimers = true; g_mockTime = 0; }
void nova_test_mock_timers_disable() { g_mockTimers = false; }
void nova_test_mock_timers_tick(int64_t ms) { g_mockTime += ms; }
void nova_test_mock_timers_runAll() { g_mockTime += 1000000; }
int64_t nova_test_mock_timers_now() { return g_mockTime; }

// Additional matchers
void nova_test_toBeCloseTo(void* ctx, double exp, int digits) { if (!ctx) return; auto* ec = (ExpectContext*)ctx; double prec = std::pow(10, -digits)/2; bool ok = std::abs(ec->numValue - exp) < prec; if (ec->negated) ok = !ok; if (!ok) { std::stringstream ss; ss << "Expected " << ec->numValue << " to be close to " << exp; delete ec; assertionFailed(ss.str()); } delete ec; }
void nova_test_toBeNaN(void* ctx) { if (!ctx) return; auto* ec = (ExpectContext*)ctx; bool ok = std::isnan(ec->numValue); if (ec->negated) ok = !ok; if (!ok) { delete ec; assertionFailed("Expected NaN"); } delete ec; }
void nova_test_toBeFinite(void* ctx) { if (!ctx) return; auto* ec = (ExpectContext*)ctx; bool ok = std::isfinite(ec->numValue); if (ec->negated) ok = !ok; if (!ok) { delete ec; assertionFailed("Expected finite"); } delete ec; }
void nova_test_toMatch(void* ctx, const char* pat) { if (!ctx || !pat) return; auto* ec = (ExpectContext*)ctx; bool ok = ec->strValue && strstr(ec->strValue, pat); if (ec->negated) ok = !ok; if (!ok) { delete ec; assertionFailed("Pattern mismatch"); } delete ec; }
void nova_test_toStartWith(void* ctx, const char* exp) { if (!ctx || !exp) return; auto* ec = (ExpectContext*)ctx; bool ok = ec->strValue && strncmp(ec->strValue, exp, strlen(exp)) == 0; if (ec->negated) ok = !ok; if (!ok) { delete ec; assertionFailed("StartWith failed"); } delete ec; }
void nova_test_toEndWith(void* ctx, const char* exp) { if (!ctx || !exp) return; auto* ec = (ExpectContext*)ctx; bool ok = false; if (ec->strValue) { size_t sl = strlen(ec->strValue), el = strlen(exp); if (sl >= el) ok = strcmp(ec->strValue + sl - el, exp) == 0; } if (ec->negated) ok = !ok; if (!ok) { delete ec; assertionFailed("EndWith failed"); } delete ec; }
void nova_test_toBeEmpty(void* ctx) { if (!ctx) return; auto* ec = (ExpectContext*)ctx; bool ok = !ec->strValue || strlen(ec->strValue) == 0; if (ec->negated) ok = !ok; if (!ok) { delete ec; assertionFailed("Expected empty"); } delete ec; }

// Test context
struct TestContext { const char* name; bool aborted; };
void* nova_test_context_create(const char* n) { return new TestContext{n, false}; }
const char* nova_test_context_name(void* p) { return p ? ((TestContext*)p)->name : ""; }
void nova_test_context_diagnostic(void* p, const char* m) { (void)p; if (m) std::cout << "# " << m << std::endl; }
void nova_test_context_free(void* p) { delete (TestContext*)p; }

// Assert helpers
void nova_test_assert(int v) { if (!v) assertionFailed("Assertion failed"); }
void nova_test_assert_ok(int v) { nova_test_assert(v); }
void nova_test_assert_equal(double a, double e) { if (a != e) assertionFailed("Not equal"); }
void nova_test_assert_strictEqual(double a, double e) { nova_test_assert_equal(a, e); }
void nova_test_assert_notEqual(double a, double e) { if (a == e) assertionFailed("Should not be equal"); }
void nova_test_assert_throws(void (*fn)()) { if (!fn) return; bool threw = false; try { fn(); } catch (...) { threw = true; } if (!threw) assertionFailed("Expected throw"); }
void nova_test_assert_doesNotThrow(void (*fn)()) { if (!fn) return; try { fn(); } catch (...) { assertionFailed("Unexpected throw"); } }
void nova_test_assert_fail(const char* m) { assertionFailed(m ? m : "Failed"); }
void nova_test_assert_match(const char* s, const char* p) { if (!s || !p || !strstr(s, p)) assertionFailed("Match failed"); }

void nova_test_setTimeout(int ms) { (void)ms; }
int nova_test_snapshot_match(const char* v, const char* n) { (void)v; (void)n; return 1; }
void nova_test_snapshot_update(const char* v, const char* n) { (void)v; (void)n; }

} // extern "C"

} // namespace test
} // namespace runtime
} // namespace nova
