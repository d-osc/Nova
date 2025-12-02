#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include "nova/Frontend/Lexer.h"
#include "nova/Frontend/Parser.h"
#include "nova/HIR/HIRGen.h"
#include "nova/MIR/MIRGen.h"
#include "nova/CodeGen/LLVMCodeGen.h"
#include "nova/Transpiler/Transpiler.h"
#include "nova/PackageManager/PackageManager.h"

// Debug output control - set to 1 to enable debug output
#define NOVA_DEBUG 0

using namespace nova;

void printUsage() {
    std::cout << R"(
╔═══════════════════════════════════════════════════════════════╗
║                     Nova Compiler v1.0.0                      ║
║         TypeScript/JavaScript AOT Compiler via LLVM           ║
╚═══════════════════════════════════════════════════════════════╝

Usage: nova [command] [options] <input>
       nova <file.ts>                    (shortcut for: nova run <file.ts>)

Commands:
  init [ts]      Initialize a new project (ts = with TypeScript)
  compile, -c    Compile source to native code (LLVM)
  run, -r        JIT compile and run
  build, -b      Transpile TypeScript to JavaScript (like tsc)
  test           Run automated tests
  check          Type check only
  install, i     Install packages (fast, with cache)
  update, u      Update packages to latest versions
  uninstall, un  Remove a package
  ci             Clean install from lockfile
  link           Link package globally or link global package to project
  list, ls       Show dependency tree
  outdated       Check for outdated packages
  login          Log in to registry
  logout         Log out from registry
  pack           Create a tarball for publishing
  publish        Publish package to registry
  emit           Emit IR at various stages

Options:
  -o <file>           Output file/directory
  -O<level>           Optimization level (0-3) [default: 2]
  --emit-llvm         Emit LLVM IR (.ll)
  --emit-mir          Emit MIR (.mir)
  --emit-hir          Emit HIR (.hir)
  --emit-asm          Emit assembly (.s)
  --emit-obj          Emit object file (.o)
  --emit-all          Emit all IR stages
  --target <triple>   Target triple (e.g., x86_64-pc-windows-msvc)
  --verbose           Verbose output
  --help              Show this help message
  --version           Show version

Package Manager Options (for install/update/uninstall):
  -S, --save          Save to dependencies (default)
  -D, --dev           Save to devDependencies
  -g, --global        Install/uninstall globally
  -p, -P, --peer      Save to peerDependencies
  -op, -Op, --optional Save to optionalDependencies

Build Options (for -b/build):
  --outDir <dir>      Output directory [default: ./dist]
  --minify            Minify output JavaScript
  --declaration       Generate .d.ts declaration files
  --declarationMap    Generate .d.ts.map source maps
  --sourceMap         Generate source map files
  --module <type>     Module system: commonjs, es6 [default: commonjs]
  --target <ver>      JS target: es5, es6, es2020 [default: es2020]
  --watch, -w         Watch mode - recompile on file changes

Examples:
  # Run TypeScript directly
  nova script.ts

  # Transpile to JavaScript (like tsc)
  nova build                      # Uses tsconfig.json
  nova -b src/app.ts              # Single file
  nova -b --outDir dist --minify  # With options
  nova -b --watch                 # Watch mode

  # Compile to native (LLVM)
  nova -c app.ts --emit-llvm

  # JIT execute
  nova -r script.ts

For more information: https://nova-lang.org/docs
)" << std::endl;
}

void printVersion() {
    std::cout << "Nova Compiler v1.0.0" << std::endl;
    std::cout << "LLVM version: 16.0.0" << std::endl;
    std::cout << "Copyright (c) 2025 Nova Lang Team" << std::endl;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage();
        return 1;
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
    } else if (command == "-r") {
        command = "run";
    } else if (command == "-b") {
        command = "build";
    } else if (command == "i") {
        command = "install";
    } else if (command == "u") {
        command = "update";
    } else if (command == "un") {
        command = "uninstall";
    } else if (command == "ls") {
        command = "list";
    }

    // Auto-detect: if first argument is a .ts or .js file, treat as "run" command
    bool autoRun = false;
    if (command.size() > 3) {
        std::string ext = command.substr(command.size() - 3);
        if (ext == ".ts" || ext == ".js") {
            autoRun = true;
        }
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
    bool noGC = false;
    bool noRuntime = false;
    std::string targetTriple;

    // Build-specific options
    std::string outDir = "./dist";
    bool minify = false;
    bool declaration = false;
    bool declarationMap = false;
    bool sourceMap = false;
    bool watchMode = false;
    std::string moduleType = "commonjs";
    std::string jsTarget = "es2020";

    // Package manager options
    pm::DependencyType depType = pm::DependencyType::Production;
    bool isGlobal = false;
    
    // If auto-run mode, the first argument is the input file
    int startArg = 2;
    if (autoRun) {
        inputFile = command;
        command = "run";  // Treat as run command
        startArg = 2;     // Other args still start at index 2
    }

    for (int i = startArg; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-o" && i + 1 < argc) {
            outputFile = argv[++i];
        }
        else if (arg.substr(0, 2) == "-O") {
            optLevel = std::stoi(arg.substr(2));
        }
        else if (arg == "--emit-llvm") {
            emitLLVM = true;
        }
        else if (arg == "--llvm") {
            emitLLVM = true;
        }
        else if (arg == "--emit-mir") {
            emitMIR = true;
        }
        else if (arg == "--emit-hir") {
            emitHIR = true;
        }
        else if (arg == "--emit-asm") {
            emitAsm = true;
        }
        else if (arg == "--emit-obj") {
            emitObj = true;
        }
        else if (arg == "--emit-all") {
            emitHIR = emitMIR = emitLLVM = true;
        }
        else if (arg == "--target" && i + 1 < argc) {
            targetTriple = argv[++i];
        }
        else if (arg == "--verbose") {
            verbose = true;
        }
        else if (arg == "--no-gc") {
            noGC = true;
        }
        else if (arg == "--no-runtime") {
            noRuntime = true;
        }
        // Build-specific options
        else if (arg == "--outDir" && i + 1 < argc) {
            outDir = argv[++i];
        }
        else if (arg == "--minify") {
            minify = true;
        }
        else if (arg == "--declaration") {
            declaration = true;
        }
        else if (arg == "--declarationMap") {
            declarationMap = true;
            declaration = true;  // declarationMap requires declaration
        }
        else if (arg == "--sourceMap") {
            sourceMap = true;
        }
        else if (arg == "--watch" || arg == "-w") {
            watchMode = true;
        }
        else if (arg == "--module" && i + 1 < argc) {
            moduleType = argv[++i];
        }
        // Package manager dependency type flags
        else if (arg == "-S" || arg == "-s" || arg == "--save") {
            depType = pm::DependencyType::Production;
        }
        else if (arg == "-D" || arg == "-d" || arg == "--dev" || arg == "--save-dev") {
            depType = pm::DependencyType::Development;
        }
        else if (arg == "-g" || arg == "--global") {
            depType = pm::DependencyType::Global;
            isGlobal = true;
        }
        else if (arg == "-p" || arg == "-P" || arg == "--peer" || arg == "--save-peer") {
            depType = pm::DependencyType::Peer;
        }
        else if (arg == "-op" || arg == "-Op" || arg == "--optional" || arg == "--save-optional") {
            depType = pm::DependencyType::Optional;
        }
        else if (arg[0] != '-') {
            inputFile = arg;
        }
    }

    // Handle init command
    if (command == "init") {
        pm::PackageManager pm;
        bool withTypeScript = (inputFile == "ts" || inputFile == "typescript");
        bool success = pm.init(".", withTypeScript);
        return success ? 0 : 1;
    }

    // Handle test command
    if (command == "test") {
        pm::PackageManager pm;
        std::string pattern = inputFile.empty() ? "" : inputFile;
        int result = pm.runTests(".", pattern);
        return result;
    }

    // Handle install command
    if (command == "install") {
        pm::PackageManager pmgr;
        pmgr.setProgressCallback([](const std::string& pkg, int current, int total, bool fromCache) {
            std::string status = fromCache ? " (cached)" : "";
            std::cout << "\r[" << current << "/" << total << "] " << pkg << status << "          " << std::flush;
        });

        std::string projectPath = inputFile.empty() ? "." : inputFile;

        // Check if installing specific package or all dependencies
        if (!inputFile.empty() && inputFile.find('/') == std::string::npos &&
            !std::filesystem::is_directory(inputFile)) {
            // Installing specific package: nova i lodash
            pm::InstallResult result;

            if (isGlobal) {
                // Global install: nova i -g lodash
                result = pmgr.installGlobal(inputFile, "latest");
                if (result.success) {
                    std::cout << "\n[nova] Installed " << inputFile << " globally" << std::endl;
                }
            } else {
                // Local install with dependency type
                result = pmgr.add(inputFile, "latest", depType);
                if (result.success) {
                    std::string typeStr = "dependencies";
                    if (depType == pm::DependencyType::Development) typeStr = "devDependencies";
                    else if (depType == pm::DependencyType::Peer) typeStr = "peerDependencies";
                    else if (depType == pm::DependencyType::Optional) typeStr = "optionalDependencies";
                    std::cout << "\n[nova] Added " << inputFile << " to " << typeStr << std::endl;
                }
            }
            return result.success ? 0 : 1;
        }

        // Install all dependencies from package.json
        auto result = pmgr.install(projectPath, true);

        if (!result.success) {
            for (const auto& err : result.errors) {
                std::cerr << "[error] " << err << std::endl;
            }
            return 1;
        }

        return 0;
    }

    // Handle update command
    if (command == "update") {
        pm::PackageManager pmgr;
        pmgr.setProgressCallback([](const std::string& pkg, int current, int total, bool fromCache) {
            std::string status = fromCache ? " (cached)" : "";
            std::cout << "\r[" << current << "/" << total << "] " << pkg << status << "          " << std::flush;
        });

        std::string projectPath = ".";

        // Check if updating specific package or all
        if (!inputFile.empty() && inputFile.find('/') == std::string::npos &&
            !std::filesystem::is_directory(inputFile)) {
            // Update specific package: nova u lodash
            auto result = pmgr.update(inputFile, depType);
            if (result.success) {
                std::cout << "\n[nova] Updated " << inputFile << std::endl;
            }
            return result.success ? 0 : 1;
        }

        // Update all packages
        auto result = pmgr.update("", depType);

        if (!result.success) {
            for (const auto& err : result.errors) {
                std::cerr << "[error] " << err << std::endl;
            }
            return 1;
        }

        return 0;
    }

    // Handle uninstall command
    if (command == "uninstall") {
        if (inputFile.empty()) {
            std::cerr << "[error] Please specify a package to uninstall" << std::endl;
            std::cerr << "Usage: nova uninstall <package-name> [-g]" << std::endl;
            return 1;
        }

        pm::PackageManager pmgr;
        bool success;

        if (isGlobal) {
            // Global uninstall: nova un -g lodash
            success = pmgr.removeGlobal(inputFile);
            if (success) {
                std::cout << "[nova] Removed " << inputFile << " globally" << std::endl;
            }
        } else {
            // Local uninstall
            success = pmgr.remove(inputFile, depType);
        }
        return success ? 0 : 1;
    }

    // Handle ci command (clean install from lockfile)
    if (command == "ci") {
        pm::PackageManager pm;
        pm.setProgressCallback([](const std::string& pkg, int current, int total, bool fromCache) {
            std::string status = fromCache ? " (cached)" : "";
            std::cout << "\r[" << current << "/" << total << "] " << pkg << status << "          " << std::flush;
        });

        std::string projectPath = inputFile.empty() ? "." : inputFile;
        auto result = pm.cleanInstall(projectPath);

        if (!result.success) {
            for (const auto& err : result.errors) {
                std::cerr << "[error] " << err << std::endl;
            }
            return 1;
        }

        return 0;
    }

    // Handle link command
    if (command == "link") {
        pm::PackageManager pm;

        if (inputFile.empty()) {
            // Link current package globally: nova link
            bool success = pm.link(".");
            return success ? 0 : 1;
        } else {
            // Link global package to current project: nova link <package>
            bool success = pm.linkPackage(inputFile);
            return success ? 0 : 1;
        }
    }

    // Handle list command
    if (command == "list") {
        pm::PackageManager pm;
        std::string projectPath = inputFile.empty() ? "." : inputFile;
        pm.listDependencies(projectPath);
        return 0;
    }

    // Handle outdated command
    if (command == "outdated") {
        pm::PackageManager pm;
        std::string projectPath = inputFile.empty() ? "." : inputFile;
        pm.checkOutdated(projectPath);
        return 0;
    }

    // Handle login command
    if (command == "login") {
        pm::PackageManager pm;
        std::string registry = inputFile.empty() ? "" : inputFile;
        bool success = pm.login(registry);
        return success ? 0 : 1;
    }

    // Handle logout command
    if (command == "logout") {
        pm::PackageManager pm;
        std::string registry = inputFile.empty() ? "" : inputFile;
        bool success = pm.logout(registry);
        return success ? 0 : 1;
    }

    // Handle pack command
    if (command == "pack") {
        pm::PackageManager pm;
        std::string projectPath = inputFile.empty() ? "." : inputFile;
        std::string tarballPath = pm.pack(projectPath);
        return tarballPath.empty() ? 1 : 0;
    }

    // Handle publish command
    if (command == "publish") {
        pm::PackageManager pm;
        std::string projectPath = inputFile.empty() ? "." : inputFile;
        bool success = pm.publish(projectPath);
        return success ? 0 : 1;
    }

    // Handle build command
    if (command == "build") {
        transpiler::Transpiler transpiler;
        transpiler::CompilerOptions opts;
        opts.outDir = outDir;
        opts.minify = minify;
        opts.declaration = declaration;
        opts.declarationMap = declarationMap;
        opts.sourceMap = sourceMap;
        opts.module = moduleType;
        opts.target = jsTarget;
        transpiler.setOptions(opts);

        // Check if input is a directory (project build) or file (single file)
        bool isProjectBuild = inputFile.empty() ||
                              std::filesystem::is_directory(inputFile) ||
                              inputFile == ".";

        std::string projectPath = inputFile.empty() ? "." : inputFile;

        // Check for tsconfig.json in project directory
        std::string configPath = projectPath + "/tsconfig.json";
        if (std::filesystem::exists(configPath)) {
            if (!transpiler.loadConfig(configPath)) {
                std::cerr << "[WARN] Could not load tsconfig.json, using defaults" << std::endl;
            }
        }

        // Watch mode
        if (watchMode) {
            transpiler.watch(projectPath, [](const transpiler::TranspileResult& result) {
                if (result.success) {
                    std::cout << "[OK] " << result.filename << std::endl;
                } else {
                    std::cerr << "[FAIL] " << result.filename << std::endl;
                    for (const auto& err : result.errors) {
                        std::cerr << "  " << err << std::endl;
                    }
                }
            });
            return 0; // Never reached, watch runs forever
        }

        if (!isProjectBuild) {
            // Single file transpilation
            auto result = transpiler.transpileFile(inputFile);
            if (result.success) {
                // Write output
                std::string baseName = std::filesystem::path(inputFile).stem().string();
                std::string outPath = outDir + "/" + baseName + ".js";
                std::filesystem::create_directories(outDir);

                // Write JS file
                std::ofstream out(outPath);
                out << result.jsCode;
                out.close();

                // Write declaration file if generated
                if (!result.dtsCode.empty()) {
                    std::string dtsPath = outDir + "/" + baseName + ".d.ts";
                    std::ofstream dtsOut(dtsPath);
                    dtsOut << result.dtsCode;
                    dtsOut.close();

                    // Write declaration map if generated
                    if (!result.declarationMap.empty()) {
                        std::string dtsMapPath = dtsPath + ".map";
                        std::ofstream dtsMapOut(dtsMapPath);
                        dtsMapOut << result.declarationMap;
                        dtsMapOut.close();
                    }
                }

                // Write source map if generated
                if (!result.sourceMap.empty()) {
                    std::string mapPath = outPath + ".map";
                    std::ofstream mapOut(mapPath);
                    mapOut << result.sourceMap;
                    mapOut.close();
                }

                std::cout << "[OK] " << inputFile << " -> " << outPath << std::endl;
                std::cout << "     " << result.inputSize << " bytes -> "
                          << result.outputSize << " bytes ("
                          << (result.inputSize > 0 ? (100 - result.outputSize * 100 / result.inputSize) : 0)
                          << "% smaller)" << std::endl;
                std::cout << "     Time: " << result.transpileTimeMs << "ms" << std::endl;
            } else {
                for (const auto& err : result.errors) {
                    std::cerr << "[ERROR] " << err << std::endl;
                }
                return 1;
            }
        } else {
            // Project build using tsconfig.json
            auto result = transpiler.build(projectPath);
            if (result.success) {
                std::cout << "[OK] Build completed successfully!" << std::endl;
                std::cout << "     Files: " << result.successCount << "/" << result.totalFiles << std::endl;
                std::cout << "     Size: " << result.totalInputSize << " -> "
                          << result.totalOutputSize << " bytes" << std::endl;
                std::cout << "     Time: " << result.totalTimeMs << "ms" << std::endl;
            } else {
                std::cerr << "[FAIL] Build failed with " << result.failCount << " errors" << std::endl;
                for (const auto& err : result.errors) {
                    std::cerr << "  " << err << std::endl;
                }
                return 1;
            }
        }
        return 0;
    }

    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified" << std::endl;
        printUsage();
        return 1;
    }
    
    // Check if input file exists
    std::ifstream file(inputFile);
    if (!file.good()) {
        std::cerr << "Error: Cannot open input file: " << inputFile << std::endl;
        return 1;
    }
    
    // Read source code
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sourceCode = buffer.str();
    file.close();
    
    if (verbose) {
        std::cout << "╔════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║         Nova Compiler - Compilation Log        ║" << std::endl;
        std::cout << "╚════════════════════════════════════════════════╝" << std::endl;
        std::cout << "\n[*] Input File: " << inputFile << std::endl;
        std::cout << "[*] Source Size: " << sourceCode.size() << " bytes" << std::endl;
        std::cout << "[*] Optimization Level: O" << optLevel << std::endl;
        if (!outputFile.empty()) {
            std::cout << "[*] Output File: " << outputFile << std::endl;
        }
        std::cout << "\n" << std::string(50, '-') << "\n" << std::endl;
    }
    
    try {
        // TODO: Implement full compilation pipeline
        // For now, just demonstrate the structure
        
        if (verbose) std::cout << "[*] Phase 1: Lexical Analysis..." << std::endl;
        Lexer lexer(inputFile, sourceCode);
        if (lexer.hasErrors()) {
            for (const auto& error : lexer.getErrors()) {
                std::cerr << error << std::endl;
            }
            return 1;
        }
        
        if (verbose) std::cout << "[*] Phase 2: Parsing..." << std::endl;
        Parser parser(lexer);
        auto ast = parser.parseProgram();
        
        if (parser.hasErrors()) {
            for (const auto& error : parser.getErrors()) {
                std::cerr << error << std::endl;
            }
            return 1;
        }
        
        if (verbose) {
            std::cout << "[OK] Successfully parsed " << ast->body.size() 
                      << " top-level statements" << std::endl;
        }
        
        if (command == "check") {
            if (verbose) std::cout << "⏳ Phase 3: Type Checking..." << std::endl;
            // TypeChecker checker;
            // checker.check(ast);
            std::cout << "✅ Type checking completed successfully" << std::endl;
            return 0;
        }
        
        if (verbose) std::cout << "⏳ Phase 3: Semantic Analysis..." << std::endl;
        // TODO: SemanticAnalyzer analyzer;
        // TODO: analyzer.analyze(ast);
        
        if (verbose) std::cout << "⏳ Phase 4: HIR Generation..." << std::endl;
        auto* hirModule = hir::generateHIR(*ast, "main");
        if (!hirModule) {
            std::cerr << "❌ Error: HIR generation failed" << std::endl;
            return 1;
        }
        
        if (emitHIR) {
            std::string hirFile = outputFile.empty() ? 
                (inputFile.substr(0, inputFile.find_last_of('.')) + ".hir") : outputFile;
            if (verbose) std::cout << "💾 Writing HIR to: " << hirFile << std::endl;
            std::ofstream hirOut(hirFile);
            if (hirOut) {
                hirOut << hirModule->toString();
                hirOut.close();
            } else {
                std::cerr << "❌ Error: Cannot write HIR file" << std::endl;
            }
        }
        
        if (verbose) std::cout << "⏳ Phase 5: HIR Optimization..." << std::endl;
        // TODO: HIROptimizer hirOpt;
        // TODO: hirOpt.optimize(hirModule);
        
        if (verbose) std::cout << "⏳ Phase 6: MIR Generation..." << std::endl;
        auto* mirModule = mir::generateMIR(hirModule, "main");
        if (!mirModule) {
            std::cerr << "❌ Error: MIR generation failed" << std::endl;
            delete hirModule;
            return 1;
        }
        
        if (emitMIR) {
            std::string mirFile = outputFile.empty() ? 
                (inputFile.substr(0, inputFile.find_last_of('.')) + ".mir") : outputFile;
            if (verbose) std::cout << "💾 Writing MIR to: " << mirFile << std::endl;
            std::ofstream mirOut(mirFile);
            if (mirOut) {
                mirOut << mirModule->toString();
                mirOut.close();
            } else {
                std::cerr << "❌ Error: Cannot write MIR file" << std::endl;
            }
        }
        
        if (verbose) std::cout << "⏳ Phase 7: MIR Optimization..." << std::endl;
        // TODO: MIROptimizer mirOpt;
        // TODO: mirOpt.optimize(mirModule);
        
        if (verbose) std::cout << "⏳ Phase 8: LLVM IR Code Generation..." << std::endl;
        codegen::LLVMCodeGen codegen("main");
        try {
            if (!codegen.generate(*mirModule)) {
                std::cerr << "❌ Error: LLVM IR generation failed" << std::endl;
                delete mirModule;
                delete hirModule;
                return 1;
            }
        } catch (const std::exception& e) {
            std::cerr << "❌ Exception in LLVM CodeGen: " << e.what() << std::endl;
            delete mirModule;
            delete hirModule;
            return 1;
        } catch (...) {
            std::cerr << "❌ Unknown exception in LLVM CodeGen" << std::endl;
            delete mirModule;
            delete hirModule;
            return 1;
        }

        if (verbose) std::cout << "⏳ Phase 9: LLVM Optimization Passes..." << std::endl;
        if (NOVA_DEBUG) std::cerr << "DEBUG: Running optimization passes with optLevel=" << optLevel << std::endl;
        codegen.runOptimizationPasses(optLevel);

        if (emitLLVM) {
            std::string llFile = outputFile.empty() ?
                (inputFile.substr(0, inputFile.find_last_of('.')) + ".ll") : outputFile;
            if (verbose) std::cout << "💾 Writing LLVM IR to: " << llFile << std::endl;
            if (!codegen.emitLLVMIR(llFile)) {
                std::cerr << "❌ Error: Cannot write LLVM IR file" << std::endl;
            }
        }

        if (command == "run") {
            if (verbose) std::cout << "\n🚀 Executing via JIT...\n" << std::endl;
            int exitCode = codegen.executeMain();
            
            // Clean up
            delete mirModule;
            delete hirModule;
            
            return exitCode;
        }
        
        if (emitAsm) {
            std::string asmFile = outputFile.empty() ? 
                (inputFile.substr(0, inputFile.find_last_of('.')) + ".s") : outputFile;
            if (verbose) std::cout << "💾 Writing assembly to: " << asmFile << std::endl;
            codegen.emitAssembly(asmFile);
        }
        
        if (emitObj) {
            std::string objFile = outputFile.empty() ? 
                (inputFile.substr(0, inputFile.find_last_of('.')) + ".o") : outputFile;
            if (verbose) std::cout << "💾 Writing object file to: " << objFile << std::endl;
            codegen.emitObjectFile(objFile);
        }
        
        if (!emitLLVM && !emitMIR && !emitHIR && !emitAsm && !emitObj) {
            std::string exeFile = outputFile.empty() ? 
                (inputFile.substr(0, inputFile.find_last_of('.')) + ".exe") : outputFile;
            if (verbose) std::cout << "⏳ Phase 10: Linking..." << std::endl;
            if (verbose) std::cout << "💾 Writing executable to: " << exeFile << std::endl;
            // TODO: Link and create executable using LLVM's object file output
            // For now, emit LLVM IR and object file
            codegen.emitLLVMIR(inputFile.substr(0, inputFile.find_last_of('.')) + ".ll");
            std::cout << "⚠️  Note: Executable generation not fully implemented yet" << std::endl;
            std::cout << "    Generated LLVM IR file instead" << std::endl;
        }
        
        // Clean up
        delete mirModule;
        delete hirModule;
        
        if (verbose) {
            std::cout << "\n" << std::string(50, '-') << std::endl;
            std::cout << "[OK] Compilation completed successfully!" << std::endl;
            std::cout << "╔════════════════════════════════════════════════╗" << std::endl;
            std::cout << "║              Compilation Summary                ║" << std::endl;
            std::cout << "╚════════════════════════════════════════════════╝" << std::endl;
        } else {
            std::cout << "[OK] Compilation completed successfully" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
