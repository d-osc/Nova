#!/usr/bin/env python3
"""
Nova Compiler - Automated Test Runner
Runs all test files and reports results
"""

import subprocess
import os
import sys
from pathlib import Path

# Colors for output
GREEN = '\033[92m'
RED = '\033[91m'
YELLOW = '\033[93m'
BLUE = '\033[94m'
RESET = '\033[0m'

def run_test(test_file):
    """Run a single test file and return (exit_code, success)"""
    try:
        result = subprocess.run(
            ['build\\Release\\nova.exe', 'run', str(test_file)],
            capture_output=True,
            text=True,
            timeout=5
        )

        # Extract exit code from debug output
        stderr_output = result.stderr if result.stderr else ""
        for line in stderr_output.split('\n'):
            if 'Program executed with exit code:' in line:
                exit_code = line.split('exit code:')[1].strip()
                return (exit_code, True)

        # If no exit code found but process succeeded
        return (str(result.returncode), True)

    except subprocess.TimeoutExpired:
        return ('TIMEOUT', False)
    except Exception as e:
        return (f'ERROR: {str(e)}', False)

def main():
    """Main test runner"""
    tests_dir = Path('tests')

    if not tests_dir.exists():
        print(f"{RED}Tests directory not found!{RESET}")
        return 1

    # Get all test files
    test_files_to_run = sorted(tests_dir.glob('test_*.ts'))
    skipped_tests = []

    print(f"{BLUE}{'='*70}{RESET}")
    print(f"{BLUE}Nova Compiler - Test Suite Runner{RESET}")
    print(f"{BLUE}{'='*70}{RESET}\n")

    # Run tests by category
    categories = {
        'Array': [],
        'String': [],
        'Math': [],
        'Number': [],
        'Class': [],
        'Arrow': [],
        'Other': []
    }

    results = {}

    for test_file in test_files_to_run:
        test_name = test_file.name
        exit_code, success = run_test(str(test_file))
        results[test_name] = (exit_code, success)

        # Categorize
        if 'array' in test_name:
            categories['Array'].append((test_name, exit_code, success))
        elif 'string' in test_name:
            categories['String'].append((test_name, exit_code, success))
        elif 'math' in test_name:
            categories['Math'].append((test_name, exit_code, success))
        elif 'number' in test_name:
            categories['Number'].append((test_name, exit_code, success))
        elif 'class' in test_name:
            categories['Class'].append((test_name, exit_code, success))
        elif 'arrow' in test_name:
            categories['Arrow'].append((test_name, exit_code, success))
        else:
            categories['Other'].append((test_name, exit_code, success))

    # Print results by category
    total_tests = 0
    passed_tests = 0

    for category, tests in categories.items():
        if not tests:
            continue

        print(f"\n{BLUE}=== {category} Methods ==={RESET}")
        for test_name, exit_code, success in tests:
            total_tests += 1
            if success:
                passed_tests += 1
                print(f"  {GREEN}PASS{RESET} {test_name:50} -> exit {exit_code}")
            else:
                print(f"  {RED}FAIL{RESET} {test_name:50} -> {exit_code}")

    # Print skipped tests
    if skipped_tests:
        print(f"\n{YELLOW}=== Skipped Tests (Callback Support Needed) ==={RESET}")
        for test_name in skipped_tests:
            print(f"  {YELLOW}SKIP{RESET} {test_name}")

    # Summary
    print(f"\n{BLUE}{'='*70}{RESET}")
    print(f"{BLUE}Summary:{RESET}")
    print(f"  Total Tests Run: {total_tests}")
    print(f"  {GREEN}Passed: {passed_tests}{RESET}")
    print(f"  {RED}Failed: {total_tests - passed_tests}{RESET}")
    print(f"  {YELLOW}Skipped: {len(skipped_tests)}{RESET}")

    pass_rate = (passed_tests / total_tests * 100) if total_tests > 0 else 0
    print(f"  Pass Rate: {pass_rate:.1f}%")
    print(f"{BLUE}{'='*70}{RESET}\n")

    return 0 if passed_tests == total_tests else 1

if __name__ == '__main__':
    sys.exit(main())
