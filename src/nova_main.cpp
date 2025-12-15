// Nova Runtime - JIT execution and interactive shell
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4100 4127 4244 4245 4267 4310 4324 4456 4458 4459 4624)
#endif

#include "nova/Frontend/Lexer.h"
#include "nova/Frontend/Parser.h"
#include "nova/HIR/HIRGen.h"
#include "nova/MIR/MIRGen.h"
#include "nova/CodeGen/LLVMCodeGen.h"
#include "nova/CodeGen/LLVMInit.h"
#include "nova/CodeGen/NativeBinaryCache.h"
#include "nova/Version.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

using namespace nova;

void printUsage() {
    std::string versionBanner = "Nova Runtime " NOVA_VERSION;
    size_t totalWidth = 63;
    size_t versionWidth = versionBanner.length();
    size_t padding = (totalWidth - versionWidth) / 2;
    std::string centeredVersion = std::string(padding, ' ') + versionBanner;

    std::cout << R"(
╔═══════════════════════════════════════════════════════════════╗
║ )" << centeredVersion << R"( ║
║         TypeScript/JavaScript Runtime via LLVM                ║
╚═══════════════════════════════════════════════════════════════╝

Usage: nova [options] <file.js|file.ts>
       nova                        (start interactive shell)

Options:
  --verbose           Verbose output
  --no-cache          Disable JIT cache
  --cache-stats       Show cache statistics
  --clear-cache       Clear JIT cache
  --help, -h          Show this help
  --version, -v       Show version

Examples:
  # Run a script
  nova script.ts
  nova app.js

  # Interactive shell
  nova

  # Clear JIT cache
  nova --clear-cache

For compilation use: novac <file.ts>
For more information: https://nova-lang.org/docs
)" << std::endl;
}

void printVersion() {
    std::cout << NOVA_VERSION_STRING << std::endl;
    std::cout << "LLVM version: 16.0.0" << std::endl;
    std::cout << "Copyright (c) 2025 Nova Lang Team" << std::endl;
}

// Interactive shell mode
int runShell() {
    std::cout << R"(
╔═══════════════════════════════════════════════════════════════╗
║                     Nova Interactive Shell                    ║
║         TypeScript/JavaScript Runtime via LLVM                ║
╚═══════════════════════════════════════════════════════════════╝
)" << std::endl;
    std::cout << "Type TypeScript/JavaScript code and press Enter to execute." << std::endl;
    std::cout << "Commands: .help, .clear, .exit" << std::endl;
    std::cout << std::endl;

    std::string line;
    std::string buffer;
    int lineNumber = 1;

    while (true) {
        if (buffer.empty()) {
            std::cout << "nova:" << lineNumber << "> ";
        } else {
            std::cout << "....:" << lineNumber << "> ";
        }

        if (!std::getline(std::cin, line)) {
            std::cout << std::endl;
            break;
        }

        if (line == ".exit" || line == ".quit" || line == ".q") {
            std::cout << "Goodbye!" << std::endl;
            break;
        }

        if (line == ".help" || line == ".h") {
            std::cout << R"(
Shell Commands:
  .help, .h     Show this help
  .clear, .c    Clear the buffer
  .exit, .q     Exit the shell
  .version      Show version info

Tips:
  - Enter single-line expressions to evaluate immediately
  - Use .clear to reset if you make a mistake
)" << std::endl;
            continue;
        }

        if (line == ".clear" || line == ".c") {
            buffer.clear();
            lineNumber = 1;
            std::cout << "Buffer cleared." << std::endl;
            continue;
        }

        if (line == ".version") {
            printVersion();
            continue;
        }

        if (line.empty()) {
            continue;
        }

        buffer += line + "\n";
        lineNumber++;

        try {
            nova::Lexer lexer("<shell>", buffer);
            if (lexer.hasErrors()) {
                for (const auto& err : lexer.getErrors()) {
                    std::cerr << err << std::endl;
                }
                buffer.clear();
                lineNumber = 1;
                continue;
            }

            nova::Parser parser(lexer);
            auto ast = parser.parseProgram();

            if (parser.hasErrors()) {
                for (const auto& err : parser.getErrors()) {
                    std::cerr << err << std::endl;
                }
                buffer.clear();
                lineNumber = 1;
                continue;
            }

            if (ast->body.empty()) {
                buffer.clear();
                lineNumber = 1;
                continue;
            }

            auto* hirModule = nova::hir::generateHIR(*ast, "shell");
            if (!hirModule) {
                std::cerr << "Error: HIR generation failed" << std::endl;
                buffer.clear();
                lineNumber = 1;
                continue;
            }

            auto* mirModule = nova::mir::generateMIR(hirModule, "shell");
            if (!mirModule) {
                std::cerr << "Error: MIR generation failed" << std::endl;
                delete hirModule;
                buffer.clear();
                lineNumber = 1;
                continue;
            }

            nova::codegen::LLVMCodeGen codegen("shell");
            if (!codegen.generate(*mirModule)) {
                std::cerr << "Error: Code generation failed" << std::endl;
                delete mirModule;
                delete hirModule;
                buffer.clear();
                lineNumber = 1;
                continue;
            }

            codegen.runOptimizationPasses(2);
            codegen.executeMain();

            delete mirModule;
            delete hirModule;

            buffer.clear();
            lineNumber = 1;

        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            buffer.clear();
            lineNumber = 1;
        }
    }

    return 0;
}

int main(int argc, char** argv) {
    // No arguments - enter interactive shell
    if (argc < 2) {
        return runShell();
    }

    std::string arg = argv[1];

    if (arg == "--help" || arg == "-h") {
        printUsage();
        return 0;
    }

    if (arg == "--version" || arg == "-v") {
        printVersion();
        return 0;
    }

    if (arg == "--cache-stats") {
        auto stats = codegen::getNativeBinaryCache().getStats();
        std::cout << "[nova] JIT Cache Statistics:" << std::endl;
        std::cout << "  Cached Executables: " << stats.numBinaries << std::endl;
        std::cout << "  Total Size: " << (stats.totalSize / 1024) << " KB" << std::endl;
        return 0;
    }

    if (arg == "--clear-cache") {
        codegen::getNativeBinaryCache().clearCache();
        std::cout << "[nova] JIT cache cleared" << std::endl;
        return 0;
    }

    // Parse options
    std::string inputFile;
    bool verbose = false;
    bool noCache = false;

    for (int i = 1; i < argc; ++i) {
        std::string option = argv[i];
        if (option == "--verbose") {
            verbose = true;
        } else if (option == "--no-cache") {
            noCache = true;
        } else if (option[0] != '-') {
            inputFile = option;
        }
    }

    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified" << std::endl;
        printUsage();
        return 1;
    }

    // Check if file exists
    std::ifstream file(inputFile);
    if (!file.good()) {
        std::cerr << "Error: Cannot open file: " << inputFile << std::endl;
        return 1;
    }

    // Read source code
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sourceCode = buffer.str();
    file.close();

    // Fast path: Check native executable cache
    if (!noCache) {
        auto& binCache = codegen::getNativeBinaryCache();
        std::string cachedExe = binCache.getCachedExePath(inputFile, sourceCode);

        if (binCache.hasCachedExecutable(cachedExe)) {
            if (verbose) {
                std::cout << "[nova] Cache HIT: " << cachedExe << std::endl;
            }
            return binCache.executeCached(cachedExe);
        } else if (verbose) {
            std::cout << "[nova] Cache MISS: compiling..." << std::endl;
        }
    }

    try {
        if (verbose) std::cout << "[nova] Lexical analysis..." << std::endl;
        Lexer lexer(inputFile, sourceCode);
        if (lexer.hasErrors()) {
            for (const auto& error : lexer.getErrors()) {
                std::cerr << error << std::endl;
            }
            return 1;
        }

        if (verbose) std::cout << "[nova] Parsing..." << std::endl;
        Parser parser(lexer);
        auto ast = parser.parseProgram();

        if (parser.hasErrors()) {
            for (const auto& error : parser.getErrors()) {
                std::cerr << error << std::endl;
            }
            return 1;
        }

        if (verbose) std::cout << "[nova] HIR generation..." << std::endl;
        auto* hirModule = hir::generateHIR(*ast, "main");
        if (!hirModule) {
            std::cerr << "Error: HIR generation failed" << std::endl;
            return 1;
        }

        if (verbose) std::cout << "[nova] MIR generation..." << std::endl;
        auto* mirModule = mir::generateMIR(hirModule, "main");
        if (!mirModule) {
            std::cerr << "Error: MIR generation failed" << std::endl;
            delete hirModule;
            return 1;
        }

        if (verbose) std::cout << "[nova] LLVM code generation..." << std::endl;
        codegen::LLVMCodeGen codegen("main");
        if (!codegen.generate(*mirModule)) {
            std::cerr << "Error: Code generation failed" << std::endl;
            delete mirModule;
            delete hirModule;
            return 1;
        }

        if (verbose) std::cout << "[nova] Running optimizations..." << std::endl;
        codegen.runOptimizationPasses(2);

        // Try to compile to native and cache
        if (!noCache) {
            auto& binCache = codegen::getNativeBinaryCache();
            std::string cachedExe = binCache.getCachedExePath(inputFile, sourceCode);

            if (verbose) std::cout << "[nova] Compiling to native: " << cachedExe << std::endl;

            if (codegen.emitExecutable(cachedExe)) {
                if (verbose) std::cout << "[nova] Executing..." << std::endl;
                delete mirModule;
                delete hirModule;
                return binCache.executeCached(cachedExe);
            }

            if (verbose) std::cerr << "[nova] Native compilation failed, using JIT..." << std::endl;
        }

        if (verbose) std::cout << "[nova] Executing via JIT..." << std::endl;
        int exitCode = codegen.executeMain();

        delete mirModule;
        delete hirModule;

        return exitCode;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
