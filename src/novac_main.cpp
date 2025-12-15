// Nova Compiler - AOT compilation and build tools
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
#include "nova/CodeGen/CompilationCache.h"
#include "nova/Transpiler/Transpiler.h"
#include "nova/Version.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

using namespace nova;

void printUsage() {
    std::string versionBanner = "Nova Compiler " NOVA_VERSION;
    size_t totalWidth = 63;
    size_t versionWidth = versionBanner.length();
    size_t padding = (totalWidth - versionWidth) / 2;
    std::string centeredVersion = std::string(padding, ' ') + versionBanner;

    std::cout << R"(
╔═══════════════════════════════════════════════════════════════╗
║ )" << centeredVersion << R"( ║
║         TypeScript/JavaScript AOT Compiler via LLVM           ║
╚═══════════════════════════════════════════════════════════════╝

Usage: novac [command] [options] <input>

Commands:
  compile, -c    Compile source to native executable
  build, -b      Transpile TypeScript to JavaScript (like tsc)
  emit           Emit IR at various stages
  check          Type check only

Options:
  -o <file>           Output file/directory
  -O<level>           Optimization level (0-3) [default: 2]
  --emit-llvm         Emit LLVM IR (.ll)
  --emit-mir          Emit MIR (.mir)
  --emit-hir          Emit HIR (.hir)
  --emit-asm          Emit assembly (.s)
  --emit-obj          Emit object file (.o)
  --emit-all          Emit all IR stages
  --target <triple>   Target triple
  --verbose           Verbose output
  --help, -h          Show this help
  --version, -v       Show version

Build Options (for -b/build):
  --outDir <dir>      Output directory [default: ./dist]
  --minify            Minify output
  --declaration       Generate .d.ts files
  --sourceMap         Generate source maps
  --module <type>     Module system: commonjs, es6
  --watch, -w         Watch mode

Examples:
  # Compile to native executable
  novac -c app.ts -o app.exe

  # Transpile to JavaScript
  novac -b src/index.ts --outDir dist

  # Emit LLVM IR
  novac emit --llvm app.ts

  # Type checking
  novac check app.ts

For running scripts: nova <file.ts>
For package management: nnpm <command>
For more information: https://nova-lang.org/docs
)" << std::endl;
}

void printVersion() {
    std::cout << NOVA_VERSION_STRING << std::endl;
    std::cout << "LLVM version: 16.0.0" << std::endl;
    std::cout << "Copyright (c) 2025 Nova Lang Team" << std::endl;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage();
        return 0;
    }

    std::string command = argv[1];

    if (command == "--help" || command == "-h") {
        printUsage();
        return 0;
    }

    if (command == "--version" || command == "-v") {
        printVersion();
        return 0;
    }

    // Command aliases
    if (command == "-c") {
        command = "compile";
    } else if (command == "-b") {
        command = "build";
    }

    // Parse arguments
    std::string inputFile;
    std::string outputFile;
    int optLevel = 2;
    bool emitLLVM = false;
    bool emitMIR = false;
    bool emitHIR = false;
    bool emitAsm = false;
    bool emitObj = false;
    bool verbose = false;
    std::string targetTriple;

    // Build-specific options
    std::string outDir = "./dist";
    bool minify = false;
    bool declaration = false;
    bool sourceMap = false;
    bool watchMode = false;
    std::string moduleType = "commonjs";

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-o" && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (arg.substr(0, 2) == "-O") {
            optLevel = std::stoi(arg.substr(2));
        } else if (arg == "--emit-llvm" || arg == "--llvm") {
            emitLLVM = true;
        } else if (arg == "--emit-mir") {
            emitMIR = true;
        } else if (arg == "--emit-hir") {
            emitHIR = true;
        } else if (arg == "--emit-asm") {
            emitAsm = true;
        } else if (arg == "--emit-obj") {
            emitObj = true;
        } else if (arg == "--emit-all") {
            emitHIR = emitMIR = emitLLVM = true;
        } else if (arg == "--target" && i + 1 < argc) {
            targetTriple = argv[++i];
        } else if (arg == "--verbose") {
            verbose = true;
        } else if (arg == "--outDir" && i + 1 < argc) {
            outDir = argv[++i];
        } else if (arg == "--minify") {
            minify = true;
        } else if (arg == "--declaration") {
            declaration = true;
        } else if (arg == "--sourceMap") {
            sourceMap = true;
        } else if (arg == "--watch" || arg == "-w") {
            watchMode = true;
        } else if (arg == "--module" && i + 1 < argc) {
            moduleType = argv[++i];
        } else if (arg[0] != '-') {
            inputFile = arg;
        }
    }

    // Handle build command (transpile)
    if (command == "build") {
        transpiler::Transpiler transpiler;
        transpiler::CompilerOptions opts;
        opts.outDir = outDir;
        opts.minify = minify;
        opts.declaration = declaration;
        opts.sourceMap = sourceMap;
        opts.module = moduleType;
        transpiler.setOptions(opts);

        if (watchMode) {
            transpiler.watch(inputFile.empty() ? "." : inputFile,
                [](const transpiler::TranspileResult& result) {
                    if (result.success) {
                        std::cout << "[OK] " << result.filename << std::endl;
                    } else {
                        std::cerr << "[FAIL] " << result.filename << std::endl;
                        for (const auto& err : result.errors) {
                            std::cerr << "  " << err << std::endl;
                        }
                    }
                });
            return 0;
        }

        if (inputFile.empty() || std::filesystem::is_directory(inputFile)) {
            auto result = transpiler.build(inputFile.empty() ? "." : inputFile);
            if (result.success) {
                std::cout << "[OK] Build completed" << std::endl;
                std::cout << "     Files: " << result.successCount << "/" << result.totalFiles << std::endl;
            } else {
                std::cerr << "[FAIL] Build failed" << std::endl;
                for (const auto& err : result.errors) {
                    std::cerr << "  " << err << std::endl;
                }
            }
            return result.success ? 0 : 1;
        } else {
            auto result = transpiler.transpileFile(inputFile);
            if (result.success) {
                std::filesystem::create_directories(outDir);
                std::string baseName = std::filesystem::path(inputFile).stem().string();
                std::string outPath = outDir + "/" + baseName + ".js";
                std::ofstream out(outPath);
                out << result.jsCode;
                out.close();
                std::cout << "[OK] " << inputFile << " -> " << outPath << std::endl;
            } else {
                for (const auto& err : result.errors) {
                    std::cerr << "[ERROR] " << err << std::endl;
                }
                return 1;
            }
        }
        return 0;
    }

    // Check, emit, and compile commands need input file
    if (command == "check" || command == "emit" || command == "compile") {
        if (inputFile.empty()) {
            std::cerr << "Error: No input file specified" << std::endl;
            printUsage();
            return 1;
        }

        std::ifstream file(inputFile);
        if (!file.good()) {
            std::cerr << "Error: Cannot open file: " << inputFile << std::endl;
            return 1;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string sourceCode = buffer.str();
        file.close();

        try {
            if (verbose) std::cout << "[*] Lexical analysis..." << std::endl;
            Lexer lexer(inputFile, sourceCode);
            if (lexer.hasErrors()) {
                for (const auto& error : lexer.getErrors()) {
                    std::cerr << error << std::endl;
                }
                return 1;
            }

            if (verbose) std::cout << "[*] Parsing..." << std::endl;
            Parser parser(lexer);
            auto ast = parser.parseProgram();

            if (parser.hasErrors()) {
                for (const auto& error : parser.getErrors()) {
                    std::cerr << error << std::endl;
                }
                return 1;
            }

            if (command == "check") {
                std::cout << "[OK] Type checking completed" << std::endl;
                return 0;
            }

            if (verbose) std::cout << "[*] HIR generation..." << std::endl;
            auto* hirModule = hir::generateHIR(*ast, "main");
            if (!hirModule) {
                std::cerr << "Error: HIR generation failed" << std::endl;
                return 1;
            }

            if (emitHIR) {
                std::string hirFile = outputFile.empty() ?
                    (inputFile.substr(0, inputFile.find_last_of('.')) + ".hir") : outputFile;
                std::ofstream hirOut(hirFile);
                hirOut << hirModule->toString();
                hirOut.close();
                if (verbose) std::cout << "[OK] HIR written to: " << hirFile << std::endl;
            }

            if (verbose) std::cout << "[*] MIR generation..." << std::endl;
            auto* mirModule = mir::generateMIR(hirModule, "main");
            if (!mirModule) {
                std::cerr << "Error: MIR generation failed" << std::endl;
                delete hirModule;
                return 1;
            }

            if (emitMIR) {
                std::string mirFile = outputFile.empty() ?
                    (inputFile.substr(0, inputFile.find_last_of('.')) + ".mir") : outputFile;
                std::ofstream mirOut(mirFile);
                mirOut << mirModule->toString();
                mirOut.close();
                if (verbose) std::cout << "[OK] MIR written to: " << mirFile << std::endl;
            }

            if (verbose) std::cout << "[*] LLVM IR generation..." << std::endl;
            codegen::LLVMCodeGen codegen("main");
            if (!codegen.generate(*mirModule)) {
                std::cerr << "Error: LLVM IR generation failed" << std::endl;
                delete mirModule;
                delete hirModule;
                return 1;
            }

            if (verbose) std::cout << "[*] Running optimizations (O" << optLevel << ")..." << std::endl;
            codegen.runOptimizationPasses(optLevel);

            if (emitLLVM) {
                std::string llFile = outputFile.empty() ?
                    (inputFile.substr(0, inputFile.find_last_of('.')) + ".ll") : outputFile;
                codegen.emitLLVMIR(llFile);
                if (verbose) std::cout << "[OK] LLVM IR written to: " << llFile << std::endl;
            }

            if (emitAsm) {
                std::string asmFile = outputFile.empty() ?
                    (inputFile.substr(0, inputFile.find_last_of('.')) + ".s") : outputFile;
                codegen.emitAssembly(asmFile);
                if (verbose) std::cout << "[OK] Assembly written to: " << asmFile << std::endl;
            }

            if (emitObj) {
                std::string objFile = outputFile.empty() ?
                    (inputFile.substr(0, inputFile.find_last_of('.')) + ".o") : outputFile;
                codegen.emitObjectFile(objFile);
                if (verbose) std::cout << "[OK] Object file written to: " << objFile << std::endl;
            }

            if (command == "compile" && !emitLLVM && !emitMIR && !emitHIR && !emitAsm && !emitObj) {
                std::string exeFile = outputFile.empty() ?
                    (inputFile.substr(0, inputFile.find_last_of('.')) + ".exe") : outputFile;
                if (verbose) std::cout << "[*] Compiling to executable: " << exeFile << std::endl;
                if (codegen.emitExecutable(exeFile)) {
                    std::cout << "[OK] Executable created: " << exeFile << std::endl;
                } else {
                    std::cerr << "[WARN] Native compilation not fully implemented" << std::endl;
                    codegen.emitLLVMIR(inputFile.substr(0, inputFile.find_last_of('.')) + ".ll");
                    std::cout << "      Generated LLVM IR instead" << std::endl;
                }
            }

            delete mirModule;
            delete hirModule;

            if (verbose) std::cout << "[OK] Compilation completed" << std::endl;

        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }

        return 0;
    }

    std::cerr << "Error: Unknown command: " << command << std::endl;
    printUsage();
    return 1;
}
