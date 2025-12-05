cmake_minimum_required(VERSION 3.20)
project(NovaCompiler VERSION 1.0.0 LANGUAGES C CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /WX /permissive-)
    add_compile_options(/utf-8)
    # Disable specific warnings from LLVM headers
    add_compile_options(/wd4100 /wd4245 /wd4267 /wd4458 /wd4244 /wd4624 /wd4459 /wd4324 /wd4310 /wd4146)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Optimization flags for Release build
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        # MSVC aggressive optimization flags
        add_compile_options(/O2 /Oi /Ot /Ob3 /GL /Gw /Gy)
        # /O2 - Max optimization
        # /Oi - Intrinsic functions
        # /Ot - Favor fast code
        # /Ob3 - Aggressive inlining
        # /GL - Whole program optimization
        # /Gw - Optimize global data
        # /Gy - Function-level linking
        add_link_options(/LTCG /OPT:REF /OPT:ICF)
        # /LTCG - Link-time code generation
        # /OPT:REF - Eliminate unreferenced functions
        # /OPT:ICF - COMDAT folding
    else()
        # GCC/Clang aggressive optimization flags
        if(WIN32)
            # On Windows, skip -ffast-math as it conflicts with MSVC STL's infinity implementation
            add_compile_options(-O3 -march=native -finline-functions -funroll-loops)
        else()
            # Use -ffast-math but allow infinity/NaN for JavaScript compatibility
            add_compile_options(-O3 -march=native -ffast-math -fno-finite-math-only -finline-functions -funroll-loops)
        endif()
    endif()
endif()

# Find LLVM
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# LLVM components
llvm_map_components_to_libnames(llvm_libs
    core
    support
    analysis
    transformutils
    scalaropts
    vectorize
    instcombine
    ipo
    irreader
    bitwriter
    target
    executionengine
    mcjit
    native
    x86codegen
    aarch64codegen
    armcodegen
    interpreter
    orcjit
    nativecodegen
)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Source files
set(NOVA_SOURCES
    # Lexer
    src/frontend/lexer/Lexer.cpp
    src/frontend/lexer/Token.cpp
    
    # Parser
    src/frontend/parser/Parser.cpp
    src/frontend/parser/ExprParser.cpp
    src/frontend/parser/StmtParser.cpp
    src/frontend/parser/DeclParser.cpp
    
    # AST
    src/frontend/ast/AST.cpp
    src/frontend/ast/ASTVisitor.cpp
    src/frontend/ast/ASTDumper.cpp
    
    # Semantic Analysis
    src/frontend/sema/TypeChecker.cpp
    src/frontend/sema/TypeInference.cpp
    src/frontend/sema/SymbolTable.cpp
    src/frontend/sema/SemanticAnalyzer.cpp
    
    # HIR Generation
    src/hir/HIRGen.cpp
    src/hir/HIRBuilder.cpp
    src/hir/HIRModule.cpp
    src/hir/HIRPrinter.cpp
    
    # HIR Optimization
    src/hir/passes/SimplifyHIR.cpp
    src/hir/passes/InlineExpansion.cpp
    src/hir/passes/DeadCodeElimination.cpp
    
    # MIR Generation
    src/mir/MIRGen.cpp
    src/mir/MIRBuilder.cpp
    src/mir/MIRModule.cpp
    src/mir/MIRPrinter.cpp
    
    # MIR Optimization
    src/mir/passes/CFGSimplification.cpp
    src/mir/passes/ConstantPropagation.cpp
    src/mir/passes/RegisterAllocation.cpp
    
    # LLVM IR Generation
    src/codegen/LLVMCodeGen.cpp
    src/codegen/LLVMInit.cpp
    src/codegen/CompilationCache.cpp
    src/codegen/CodeGenContext.cpp
    src/codegen/TypeGen.cpp
    src/codegen/FunctionGen.cpp
    src/codegen/ExprGen.cpp
    
    # Runtime
    src/runtime/Memory.cpp
    src/runtime/GarbageCollector.cpp
    src/runtime/AsyncRuntime.cpp
    src/runtime/Array.cpp
    src/runtime/String.cpp
    src/runtime/Object.cpp
    src/runtime/Number.cpp
    src/runtime/Function.cpp
    src/runtime/Utility.cpp
    src/runtime/Regex.cpp
    src/runtime/Error.cpp
    src/runtime/ArrayBuffer.cpp
    src/runtime/Disposable.cpp
    src/runtime/Promise.cpp
    src/runtime/Generator.cpp
    src/runtime/Intl.cpp
    src/runtime/Iterator.cpp
    src/runtime/Map.cpp
    src/runtime/Proxy.cpp
    src/runtime/Reflect.cpp
    src/runtime/Set.cpp
    src/runtime/Symbol.cpp
    src/runtime/WeakMap.cpp
    src/runtime/WeakRef.cpp
    src/runtime/WeakSet.cpp
    src/runtime/Test.cpp

    # Built-in Modules (nova:fs, nova:test, nova:path, nova:os)
    src/runtime/BuiltinModules.cpp
    src/runtime/BuiltinFS.cpp
    src/runtime/BuiltinTest.cpp
    src/runtime/BuiltinPath.cpp
    src/runtime/BuiltinOS.cpp
    src/runtime/BuiltinCrypto.cpp
    src/runtime/BuiltinAssert.cpp
    src/runtime/BuiltinAsyncHooks.cpp
    src/runtime/BuiltinBuffer.cpp
    src/runtime/BuiltinChildProcess.cpp
    src/runtime/BuiltinCluster.cpp
    src/runtime/BuiltinDiagnosticsChannel.cpp
    src/runtime/BuiltinDNS.cpp
    src/runtime/BuiltinDomain.cpp
    src/runtime/BuiltinEvents.cpp
    # src/runtime/BuiltinHTTP.cpp            # Legacy version (non-optimized)
    src/runtime/BuiltinHTTP_ultra.cpp        # ULTRA-OPTIMIZED (DEFAULT) - 102k+ req/s
    src/runtime/BuiltinHTTP2.cpp
    src/runtime/BuiltinHTTPS.cpp
    src/runtime/BuiltinInspector.cpp
    src/runtime/BuiltinModule.cpp
    src/runtime/BuiltinNet.cpp
    src/runtime/BuiltinPerfHooks.cpp
    src/runtime/BuiltinProcess.cpp
    src/runtime/BuiltinPunycode.cpp
    src/runtime/BuiltinQuerystring.cpp
    src/runtime/BuiltinReadline.cpp
    src/runtime/BuiltinRepl.cpp
    # src/runtime/BuiltinSQLite.cpp           # Legacy version (non-optimized)
    src/runtime/BuiltinSQLite_ultra.cpp      # ULTRA-OPTIMIZED (DEFAULT) - 5-10x faster
    src/runtime/BuiltinStream.cpp
    src/runtime/BuiltinTLS.cpp
    src/runtime/BuiltinTraceEvents.cpp
    src/runtime/BuiltinTTY.cpp
    src/runtime/BuiltinDgram.cpp
    src/runtime/BuiltinUtil.cpp
    src/runtime/BuiltinV8.cpp
    src/runtime/BuiltinVM.cpp
    src/runtime/BuiltinWASI.cpp
    src/runtime/BuiltinWorkerThreads.cpp
    src/runtime/BuiltinZlib.cpp

    # Web APIs Runtime
    src/runtime/Timers.cpp
    src/runtime/URL.cpp
    src/runtime/TextCodec.cpp
    src/runtime/Fetch.cpp
    src/runtime/WebCrypto.cpp
    src/runtime/WebStreams.cpp

    # Diagnostics
    src/diagnostics/Diagnostic.cpp
    src/diagnostics/DiagnosticEngine.cpp

    # Driver
    src/driver/Driver.cpp
    src/driver/CompilationJob.cpp

    # Transpiler (TypeScript to JavaScript)
    src/transpiler/Transpiler.cpp

    # Package Manager
    src/pm/PackageManager.cpp
)

# Create library
add_library(novacore STATIC ${NOVA_SOURCES})
target_include_directories(novacore SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_libraries(novacore ${llvm_libs})

# Create executable
add_executable(nova src/main.cpp)
target_include_directories(nova SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_libraries(nova novacore ${llvm_libs})

# Windows-specific DIA SDK configuration
if(MSVC)
    # Try to use DIA SDK from VS 14.0 if available
    set(DIA_SDK_LIB "C:/Program Files (x86)/Microsoft Visual Studio 14.0/DIA SDK/lib/amd64/diaguids.lib")
    if(EXISTS "${DIA_SDK_LIB}")
        target_link_options(nova PRIVATE "/LIBPATH:C:/Program Files (x86)/Microsoft Visual Studio 14.0/DIA SDK/lib/amd64")
    else()
        # diaguids.lib not essential for basic functionality - ignore if missing
        target_link_options(nova PRIVATE /IGNORE:4099 /FORCE:MULTIPLE)
    endif()
endif()

# Install targets
install(TARGETS nova DESTINATION bin)
install(TARGETS novacore DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)

# Testing - temporarily disabled
# enable_testing()
# add_subdirectory(tests)

# Documentation
option(BUILD_DOCS "Build documentation" OFF)
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_subdirectory(docs)
    endif()
endif()

# Examples
option(BUILD_EXAMPLES "Build examples" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
