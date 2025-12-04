#!/bin/bash
# Script to fix the emitExecutable function to link with novacore.lib

FILE="src/codegen/LLVMCodeGen.cpp"

# Create a backup
cp "$FILE" "$FILE.bak"

# Use awk to replace the section
awk '
/\/\/ Step 2: Compile IR to executable using clang\+\+/ {
    print "    // Step 2: Determine path to novacore library"
    print "    std::string novacoreLib;"
    print "#ifdef _WIN32"
    print "    // On Windows, find novacore.lib relative to the Nova executable"
    print "    char exePath[MAX_PATH];"
    print "    GetModuleFileNameA(NULL, exePath, MAX_PATH);"
    print "    std::string exeDir = exePath;"
    print "    size_t lastSlash = exeDir.find_last_of(\"\\\\/\");"
    print "    if (lastSlash != std::string::npos) {"
    print "        exeDir = exeDir.substr(0, lastSlash);"
    print "    }"
    print "    novacoreLib = exeDir + \"/novacore.lib\";"
    print ""
    print "    if(NOVA_DEBUG) {"
    print "        std::cerr << \"DEBUG LLVM: Looking for novacore.lib at: \" << novacoreLib << std::endl;"
    print "        std::ifstream libCheck(novacoreLib);"
    print "        std::cerr << \"DEBUG LLVM: novacore.lib exists: \" << (libCheck.good() ? \"yes\" : \"no\") << std::endl;"
    print "    }"
    print "#else"
    print "    // On Unix, use relative path"
    print "    novacoreLib = \"build/Release/libnovacore.a\";"
    print "#endif"
    print ""
    print "    // Step 3: Compile IR to executable using clang++ with runtime library"
    print "    std::string compileCmd;"
    print "#ifdef _WIN32"
    print "    compileCmd = \"clang++ -O2 \\\"\" + irFile + \"\\\" \\\"\" + novacoreLib + \"\\\" -o \\\"\" + filename + \"\\\" -lmsvcrt -lkernel32 -lWs2_32 -lAdvapi32 -Wno-override-module 2>&1\";"
    print "#else"
    print "    compileCmd = \"clang++ -O2 \\\"\" + irFile + \"\\\" \\\"\" + novacoreLib + \"\\\" -o \\\"\" + filename + \"\\\" -lc -lstdc++ 2>&1\";"
    print "#endif"
    getline # Skip the original line
    next
}
/std::string compileCmd = "clang\+\+ -O2/ { next } # Skip the old compileCmd line
{ print }
' "$FILE.bak" > "$FILE"

echo "File updated successfully!"
