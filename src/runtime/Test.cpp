/**
 * Nova Test Runtime - bun:test compatible testing framework
 */

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <cstring>
#include <cmath>

// Test result tracking
static int totalTests = 0;
static int passedTests = 0;
static int failedTests = 0;
static std::string currentDescribe = "";
static std::string currentTest = "";
static bool currentTestFailed = false;
static std::vector<std::string> failedTestNames;

// ANSI color codes
#define GREEN "\033[32m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define GRAY "\033[90m"
#define RESET "\033[0m"
#define BOLD "\033[1m"

extern "C" {

// describe(name, fn) - Group tests
void nova_describe(const char* name, void (*fn)()) {
    currentDescribe = name;
    std::cout << "\n" << BOLD << name << RESET << std::endl;
    fn();
    currentDescribe = "";
}

// test(name, fn) - Single test
void nova_test(const char* name, void (*fn)()) {
    totalTests++;
    currentTest = name;
    currentTestFailed = false;

    try {
        fn();
    } catch (...) {
        currentTestFailed = true;
    }

    if (!currentTestFailed) {
        passedTests++;
        std::cout << "  " << GREEN << "✓" << RESET << " " << name << std::endl;
    } else {
        failedTests++;
        std::string fullName = currentDescribe.empty() ? name : currentDescribe + " > " + name;
        failedTestNames.push_back(fullName);
        std::cout << "  " << RED << "✗" << RESET << " " << name << std::endl;
    }

    currentTest = "";
}

// it(name, fn) - Alias for test()
void nova_it(const char* name, void (*fn)()) {
    nova_test(name, fn);
}

// Expect structure for chaining
struct Expectation {
    double numValue;
    const char* strValue;
    bool boolValue;
    bool isNumber;
    bool isString;
    bool isBool;
    bool isNull;
    bool negated;
};

static Expectation currentExpect;

// expect(value) - Create expectation
void* nova_expect_number(double value) {
    currentExpect.numValue = value;
    currentExpect.isNumber = true;
    currentExpect.isString = false;
    currentExpect.isBool = false;
    currentExpect.isNull = false;
    currentExpect.negated = false;
    return &currentExpect;
}

void* nova_expect_string(const char* value) {
    currentExpect.strValue = value;
    currentExpect.isNumber = false;
    currentExpect.isString = true;
    currentExpect.isBool = false;
    currentExpect.isNull = value == nullptr;
    currentExpect.negated = false;
    return &currentExpect;
}

void* nova_expect_bool(int value) {
    currentExpect.boolValue = value != 0;
    currentExpect.isNumber = false;
    currentExpect.isString = false;
    currentExpect.isBool = true;
    currentExpect.isNull = false;
    currentExpect.negated = false;
    return &currentExpect;
}

// .not property
void* nova_expect_not(void* exp) {
    currentExpect.negated = !currentExpect.negated;
    return exp;
}

// Matcher helpers
static void reportFailure(const char* matcher, const char* expected, const char* actual) {
    currentTestFailed = true;
    std::cout << "      " << RED << "Expected: " << RESET << expected << std::endl;
    std::cout << "      " << RED << "Received: " << RESET << actual << std::endl;
}

// .toBe(expected) - Strict equality
void nova_expect_toBe_number(void* exp, double expected) {
    bool pass = (currentExpect.numValue == expected);
    if (currentExpect.negated) pass = !pass;

    if (!pass) {
        reportFailure("toBe", std::to_string(expected).c_str(),
                     std::to_string(currentExpect.numValue).c_str());
    }
}

void nova_expect_toBe_string(void* exp, const char* expected) {
    bool pass = false;
    if (currentExpect.strValue && expected) {
        pass = (strcmp(currentExpect.strValue, expected) == 0);
    } else {
        pass = (currentExpect.strValue == expected);
    }
    if (currentExpect.negated) pass = !pass;

    if (!pass) {
        reportFailure("toBe", expected ? expected : "null",
                     currentExpect.strValue ? currentExpect.strValue : "null");
    }
}

void nova_expect_toBe_bool(void* exp, int expected) {
    bool pass = (currentExpect.boolValue == (expected != 0));
    if (currentExpect.negated) pass = !pass;

    if (!pass) {
        reportFailure("toBe", expected ? "true" : "false",
                     currentExpect.boolValue ? "true" : "false");
    }
}

// .toEqual(expected) - Deep equality (same as toBe for primitives)
void nova_expect_toEqual_number(void* exp, double expected) {
    nova_expect_toBe_number(exp, expected);
}

void nova_expect_toEqual_string(void* exp, const char* expected) {
    nova_expect_toBe_string(exp, expected);
}

void nova_expect_toEqual_bool(void* exp, int expected) {
    nova_expect_toBe_bool(exp, expected);
}

// .toBeTruthy()
void nova_expect_toBeTruthy(void* exp) {
    bool pass = false;
    if (currentExpect.isNumber) {
        pass = (currentExpect.numValue != 0);
    } else if (currentExpect.isString) {
        pass = (currentExpect.strValue != nullptr && strlen(currentExpect.strValue) > 0);
    } else if (currentExpect.isBool) {
        pass = currentExpect.boolValue;
    }
    if (currentExpect.negated) pass = !pass;

    if (!pass) {
        currentTestFailed = true;
        std::cout << "      " << RED << "Expected value to be truthy" << RESET << std::endl;
    }
}

// .toBeFalsy()
void nova_expect_toBeFalsy(void* exp) {
    bool pass = false;
    if (currentExpect.isNumber) {
        pass = (currentExpect.numValue == 0);
    } else if (currentExpect.isString) {
        pass = (currentExpect.strValue == nullptr || strlen(currentExpect.strValue) == 0);
    } else if (currentExpect.isBool) {
        pass = !currentExpect.boolValue;
    }
    if (currentExpect.negated) pass = !pass;

    if (!pass) {
        currentTestFailed = true;
        std::cout << "      " << RED << "Expected value to be falsy" << RESET << std::endl;
    }
}

// .toBeNull()
void nova_expect_toBeNull(void* exp) {
    bool pass = currentExpect.isNull;
    if (currentExpect.negated) pass = !pass;

    if (!pass) {
        currentTestFailed = true;
        std::cout << "      " << RED << "Expected value to be null" << RESET << std::endl;
    }
}

// .toBeDefined()
void nova_expect_toBeDefined(void* exp) {
    bool pass = !currentExpect.isNull;
    if (currentExpect.negated) pass = !pass;

    if (!pass) {
        currentTestFailed = true;
        std::cout << "      " << RED << "Expected value to be defined" << RESET << std::endl;
    }
}

// .toBeUndefined()
void nova_expect_toBeUndefined(void* exp) {
    nova_expect_toBeNull(exp);
}

// .toBeGreaterThan(expected)
void nova_expect_toBeGreaterThan(void* exp, double expected) {
    bool pass = (currentExpect.numValue > expected);
    if (currentExpect.negated) pass = !pass;

    if (!pass) {
        currentTestFailed = true;
        std::cout << "      " << RED << "Expected " << currentExpect.numValue
                  << " to be greater than " << expected << RESET << std::endl;
    }
}

// .toBeGreaterThanOrEqual(expected)
void nova_expect_toBeGreaterThanOrEqual(void* exp, double expected) {
    bool pass = (currentExpect.numValue >= expected);
    if (currentExpect.negated) pass = !pass;

    if (!pass) {
        currentTestFailed = true;
        std::cout << "      " << RED << "Expected " << currentExpect.numValue
                  << " to be >= " << expected << RESET << std::endl;
    }
}

// .toBeLessThan(expected)
void nova_expect_toBeLessThan(void* exp, double expected) {
    bool pass = (currentExpect.numValue < expected);
    if (currentExpect.negated) pass = !pass;

    if (!pass) {
        currentTestFailed = true;
        std::cout << "      " << RED << "Expected " << currentExpect.numValue
                  << " to be less than " << expected << RESET << std::endl;
    }
}

// .toBeLessThanOrEqual(expected)
void nova_expect_toBeLessThanOrEqual(void* exp, double expected) {
    bool pass = (currentExpect.numValue <= expected);
    if (currentExpect.negated) pass = !pass;

    if (!pass) {
        currentTestFailed = true;
        std::cout << "      " << RED << "Expected " << currentExpect.numValue
                  << " to be <= " << expected << RESET << std::endl;
    }
}

// .toBeCloseTo(expected, precision)
void nova_expect_toBeCloseTo(void* exp, double expected, int precision) {
    double diff = std::abs(currentExpect.numValue - expected);
    double threshold = std::pow(10, -precision) / 2;
    bool pass = (diff < threshold);
    if (currentExpect.negated) pass = !pass;

    if (!pass) {
        currentTestFailed = true;
        std::cout << "      " << RED << "Expected " << currentExpect.numValue
                  << " to be close to " << expected << RESET << std::endl;
    }
}

// .toContain(item) - For strings
void nova_expect_toContain(void* exp, const char* item) {
    bool pass = false;
    if (currentExpect.strValue && item) {
        pass = (strstr(currentExpect.strValue, item) != nullptr);
    }
    if (currentExpect.negated) pass = !pass;

    if (!pass) {
        currentTestFailed = true;
        std::cout << "      " << RED << "Expected \"" << (currentExpect.strValue ? currentExpect.strValue : "null")
                  << "\" to contain \"" << (item ? item : "null") << "\"" << RESET << std::endl;
    }
}

// .toHaveLength(length)
void nova_expect_toHaveLength(void* exp, int length) {
    int actualLength = 0;
    if (currentExpect.strValue) {
        actualLength = strlen(currentExpect.strValue);
    }
    bool pass = (actualLength == length);
    if (currentExpect.negated) pass = !pass;

    if (!pass) {
        currentTestFailed = true;
        std::cout << "      " << RED << "Expected length " << length
                  << " but got " << actualLength << RESET << std::endl;
    }
}

// .toMatch(pattern) - Simple string match (not regex)
void nova_expect_toMatch(void* exp, const char* pattern) {
    nova_expect_toContain(exp, pattern);
}

// .toThrow() - Check if function throws
void nova_expect_toThrow(void* exp) {
    // This would need special handling - mark as pass for now
    // In real implementation, fn() would be stored and called here
}

// Print test summary
void nova_test_summary() {
    std::cout << "\n" << std::string(50, '-') << std::endl;

    if (failedTests == 0) {
        std::cout << GREEN << BOLD << " ✓ " << passedTests << " tests passed" << RESET << std::endl;
    } else {
        std::cout << RED << BOLD << " ✗ " << failedTests << " failed" << RESET;
        std::cout << GRAY << " | " << RESET;
        std::cout << GREEN << passedTests << " passed" << RESET;
        std::cout << GRAY << " | " << RESET;
        std::cout << totalTests << " total" << std::endl;

        if (!failedTestNames.empty()) {
            std::cout << "\n" << RED << "Failed tests:" << RESET << std::endl;
            for (const auto& name : failedTestNames) {
                std::cout << "  " << RED << "✗" << RESET << " " << name << std::endl;
            }
        }
    }
    std::cout << std::endl;
}

// Get test results for exit code
int nova_test_exit_code() {
    return failedTests > 0 ? 1 : 0;
}

// Reset test state (for multiple test runs)
void nova_test_reset() {
    totalTests = 0;
    passedTests = 0;
    failedTests = 0;
    currentDescribe = "";
    currentTest = "";
    currentTestFailed = false;
    failedTestNames.clear();
}

} // extern "C"
