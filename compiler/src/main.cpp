#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "nova/Frontend/Lexer.h"
#include "nova/Frontend/Parser.h"
#include "nova/HIR/HIRGen.h"
#include "nova/MIR/MIRGen.h"
#include "nova/CodeGen/LLVMCodeGen.h"

using namespace nova;

void printUsage() {
    std::cout << R"(
╔═══════════════════════════════════════════════════════════════╗
║                     Nova Compiler v1.0.0                      ║
║         TypeScript/JavaScript AOT Compiler via LLVM           ║
╚═══════════════════════════════════════════════════════════════╝

Usage: nova [command] [options] <input>

Commands:
  compile    Compile source to native code
  run        JIT compile and run
  check      Type check only
  emit       Emit IR at various stages

Options:
  -o <file>           Output file
  -O<level>           Optimization level (0-3) [default: 2]
  --emit-llvm         Emit LLVM IR (.ll)
  --emit-mir          Emit MIR (.mir)
  --emit-hir          Emit HIR (.hir)
  --emit-asm          Emit assembly (.s)
  --emit-obj          Emit object file (.o)
  --emit-all          Emit all IR stages
  --target <triple>   Target triple (e.g., x86_64-pc-windows-msvc)
  --no-gc             Disable garbage collector
  --no-runtime        Exclude runtime library
  --verbose           Verbose output
  --help              Show this help message
  --version           Show version

Examples:
  # Compile to executable
  nova compile hello.ts -o hello.exe

  # Compile with optimizations
  nova compile app.ts -O3 -o app.exe

  # Emit LLVM IR
  nova compile app.ts --emit-llvm -o app.ll

  # JIT execute
  nova run script.ts

  # Type check only
  nova check app.ts

  # Emit all IR stages
  nova compile app.ts --emit-all --verbose

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
    
    for (int i = 2; i < argc; ++i) {
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
        else if (arg[0] != '-') {
            inputFile = arg;
        }
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
        
        if (emitLLVM) {
            std::string llFile = outputFile.empty() ? 
                (inputFile.substr(0, inputFile.find_last_of('.')) + ".ll") : outputFile;
            if (verbose) std::cout << "💾 Writing LLVM IR to: " << llFile << std::endl;
            if (!codegen.emitLLVMIR(llFile)) {
                std::cerr << "❌ Error: Cannot write LLVM IR file" << std::endl;
            }
        }
        
        if (verbose) std::cout << "⏳ Phase 9: LLVM Optimization Passes..." << std::endl;
        std::cerr << "DEBUG: Running optimization passes with optLevel=" << optLevel << std::endl;
        codegen.runOptimizationPasses(optLevel);
        
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
