#!/usr/bin/env python3

# Read the file
with open('src/hir/HIRGen.cpp', 'r', encoding='utf-8', errors='ignore') as f:
    content = f.read()

# Find and fix the async generator result marking
old_code = '''                            // Mark that this returns a Promise (containing IteratorResult)
                            lastWasPromise_ = true;

                            std::cerr << "DEBUG HIRGen: AsyncGenerator." << methodName << "() called" << std::endl;'''

new_code = '''                            // Mark that this returns an IteratorResult (for synchronous compilation)
                            // Also mark as Promise for future full async support
                            lastWasIteratorResult_ = true;
                            lastWasPromise_ = true;

                            std::cerr << "DEBUG HIRGen: AsyncGenerator." << methodName << "() called" << std::endl;'''

if old_code in content:
    content = content.replace(old_code, new_code)
    with open('src/hir/HIRGen.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print("Fixed async generator result marking")
else:
    print("Pattern not found - may already be fixed")
