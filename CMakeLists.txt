cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0091 NEW)
project(NovaCompiler VERSION 1.4.0 LANGUAGES C CXX)

# Clear implicit include directories to prevent system header conflicts on macOS
# This must be done after project() but before any targets are created
# Only do this on macOS where we explicitly specify LLVM paths
if(APPLE)
    set(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES "")
    set(CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES "")
endif()

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(NOVA_ENABLE_ASAN "Build Nova and its tests with AddressSanitizer" OFF)

# Compiler flags
if(MSVC)
    # The official LLVM Windows archives use the release dynamic CRT even for
    # clients built with debug information. Match that ABI in every Nova
    # configuration while retaining Debug optimization/symbol settings.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    add_compile_definitions(_ITERATOR_DEBUG_LEVEL=0)
    add_compile_options(/W4 /WX /permissive-)
    add_compile_options(/utf-8)
    # Disable specific warnings from LLVM headers
    add_compile_options(/wd4100 /wd4245 /wd4267 /wd4458 /wd4244 /wd4624 /wd4459 /wd4324 /wd4310 /wd4146 /wd4127)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    # On macOS, allow #include_next extension and unused parameters in LLVM headers
    # Also disable deprecated declarations warning (sprintf, etc.)
    if(APPLE)
        add_compile_options(-Wno-gnu-include-next -Wno-unused-parameter -Wno-deprecated-declarations)
    endif()
endif()

if(NOVA_ENABLE_ASAN)
    add_compile_definitions(NOVA_ENABLE_ASAN=1)
    if(MSVC)
        add_compile_options(/fsanitize=address /Zi)
        add_link_options(/INCREMENTAL:NO /DEBUG)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        add_compile_options(
            -fsanitize=address
            -fno-omit-frame-pointer
            -fno-optimize-sibling-calls
        )
        add_link_options(-fsanitize=address)
    else()
        message(FATAL_ERROR
            "NOVA_ENABLE_ASAN is unsupported by ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Optimization flags for Release build
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        # Multi-config generators share one configure step. Scope Release-only
        # flags explicitly so Debug/ASan builds keep /RTC and symbols valid.
        if(CMAKE_CONFIGURATION_TYPES)
            add_compile_options(
                $<$<CONFIG:Release>:/O2>
                $<$<CONFIG:Release>:/Oi>
                $<$<CONFIG:Release>:/Ot>
                $<$<CONFIG:Release>:/Ob3>
                $<$<CONFIG:Release>:/GL>
                $<$<CONFIG:Release>:/Gw>
                $<$<CONFIG:Release>:/Gy>
            )
            add_link_options(
                $<$<CONFIG:Release>:/LTCG>
                $<$<CONFIG:Release>:/OPT:REF>
                $<$<CONFIG:Release>:/OPT:ICF>
            )
        else()
            add_compile_options(/O2 /Oi /Ot /Ob3 /GL /Gw /Gy)
            add_link_options(/LTCG /OPT:REF /OPT:ICF)
        endif()
        # /O2 - Max optimization
        # /Oi - Intrinsic functions
        # /Ot - Favor fast code
        # /Ob3 - Aggressive inlining
        # /GL - Whole program optimization
        # /Gw - Optimize global data
        # /Gy - Function-level linking
        # /LTCG - Link-time code generation
        # /OPT:REF - Eliminate unreferenced functions
        # /OPT:ICF - COMDAT folding
    else()
        # GCC/Clang aggressive optimization flags
        # Only add -march=native if no -march flag is already set in CMAKE_CXX_FLAGS
        set(MARCH_FLAG "-march=native")
        if(CMAKE_CXX_FLAGS MATCHES "-march=")
            set(MARCH_FLAG "")
        endif()

        if(WIN32)
            # On Windows, skip -ffast-math as it conflicts with MSVC STL's infinity implementation
            if(MARCH_FLAG)
                add_compile_options(-O3 ${MARCH_FLAG} -finline-functions -funroll-loops)
            else()
                add_compile_options(-O3 -finline-functions -funroll-loops)
            endif()
        else()
            # Use -ffast-math but allow infinity/NaN for JavaScript compatibility
            if(MARCH_FLAG)
                add_compile_options(-O3 ${MARCH_FLAG} -ffast-math -fno-finite-math-only -finline-functions -funroll-loops)
            else()
                add_compile_options(-O3 -ffast-math -fno-finite-math-only -finline-functions -funroll-loops)
            endif()
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

# Generate version header
set(VERSION_HEADER "${CMAKE_BINARY_DIR}/include/nova/Version.h")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/include/nova")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Version.h.in"
    "${VERSION_HEADER}"
    @ONLY
)

# Add the generated header to the include path
include_directories("${CMAKE_BINARY_DIR}/include")

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
    src/hir/HIRGen_Advanced.cpp
    src/hir/HIRGen_Arrays.cpp
    src/hir/HIRGen_Calls.cpp
    src/hir/HIRGen_Classes.cpp
    src/hir/HIRGen_ControlFlow.cpp
    src/hir/HIRGen_Functions.cpp
    src/hir/HIRGen_Literals.cpp
    src/hir/HIRGen_Objects.cpp
    src/hir/HIRGen_Operators.cpp
    src/hir/HIRGen_Statements.cpp
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
    src/runtime/ArraySpread.cpp
    src/runtime/ClosureEnv.cpp
    src/runtime/String.cpp
    src/runtime/Object.cpp
    src/runtime/Value.cpp
    src/runtime/Number.cpp
    src/runtime/Math.cpp
    src/runtime/Function.cpp
    src/runtime/Utility.cpp
    src/runtime/Regex.cpp
    src/runtime/Error.cpp
    src/runtime/ArrayBuffer.cpp
    src/runtime/BigInt.cpp
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
    src/runtime/Date.cpp
    src/runtime/Boolean.cpp
    src/runtime/Atomics.cpp

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
# On macOS, use regular includes (not SYSTEM) to get -I instead of -isystem
if(APPLE)
    target_include_directories(novacore PRIVATE ${LLVM_INCLUDE_DIRS})
else()
    target_include_directories(novacore SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
endif()
target_link_libraries(novacore ${llvm_libs})

# Create executable
add_executable(nova src/main.cpp)
# On macOS, use regular includes (not SYSTEM) to get -I instead of -isystem
if(APPLE)
    target_include_directories(nova PRIVATE ${LLVM_INCLUDE_DIRS})
else()
    target_include_directories(nova SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
endif()
target_link_libraries(nova novacore ${llvm_libs})
if(MSVC)
    set_property(
        TARGET novacore nova
        PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
    )
endif()

# Windows-specific DIA SDK configuration
if(MSVC)
    # diaguids.lib not essential for basic functionality - ignore if missing
    # Filter out DIA SDK library from LLVM dependencies
    set_target_properties(nova PROPERTIES LINK_FLAGS "/IGNORE:4099")
    set_target_properties(novacore PROPERTIES LINK_FLAGS "/IGNORE:4099")

    # Alternative: try to remove the library from link interface
    get_target_property(LLVM_LIBS nova LINK_LIBRARIES)
    if(LLVM_LIBS)
        list(FILTER LLVM_LIBS EXCLUDE REGEX ".*diaguids.*")
        set_target_properties(nova PROPERTIES LINK_LIBRARIES "${LLVM_LIBS}")
    endif()
endif()

# Install targets
install(TARGETS nova DESTINATION bin)
install(TARGETS novacore DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)

# Conformance tests
#
# The historical GoogleTest targets in tests/CMakeLists.txt are intentionally
# not enabled here yet because most of them are placeholders. This suite runs
# only source files that declare explicit expectations.
include(CTest)
if(BUILD_TESTING)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(
        NAME nova-conformance
        COMMAND
            ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_all_tests.py"
            --nova "$<TARGET_FILE:nova>"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/conformance"
    )
    set_tests_properties(nova-conformance PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        TIMEOUT 180
    )

    add_executable(
        nova-parser-phase1-tests
        tests/unit/ParserPhase1Test.cpp
    )
    target_link_libraries(
        nova-parser-phase1-tests
        PRIVATE novacore ${llvm_libs}
    )
    target_include_directories(
        nova-parser-phase1-tests
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_compile_definitions(
        nova-parser-phase1-tests
        PRIVATE NOVA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    if(MSVC)
        set_property(
            TARGET nova-parser-phase1-tests
            PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
        )
    endif()
    add_test(
        NAME nova-parser-phase1
        COMMAND nova-parser-phase1-tests
    )

    add_test(
        NAME nova-phase0-safety
        COMMAND
            ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_all_tests.py"
            --nova "$<TARGET_FILE:nova>"
            --cache-mode compare
            --safety-only
            --failure-debug
            "${CMAKE_CURRENT_BINARY_DIR}/phase0_fail_debug.txt"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/phase0"
    )
    set_tests_properties(nova-phase0-safety PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        TIMEOUT 180
    )

    add_test(
        NAME nova-phase5-typescript
        COMMAND
            ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_all_tests.py"
            --nova "$<TARGET_FILE:nova>"
            --prefix ts_spec_structural_generics
            --prefix ts_spec_narrowing
            --prefix ts_spec_type_operators
            --prefix ts_spec_classes_overloads
            --prefix ts_spec_negative
            --failure-debug
            "${CMAKE_CURRENT_BINARY_DIR}/phase5_fail_debug.txt"
    )
    set_tests_properties(nova-phase5-typescript PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        TIMEOUT 60
    )

    add_test(
        NAME nova-phase6-isolated
        COMMAND
            ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_all_tests.py"
            --nova "$<TARGET_FILE:nova>"
            --prefix ts_spec_declarations
            --prefix ts_spec_ambient_modules
            --prefix ts_spec_jsx
            --prefix ts_spec_module_extension
            --failure-debug
            "${CMAKE_CURRENT_BINARY_DIR}/phase6_fail_debug.txt"
    )
    add_test(
        NAME nova-phase6-project
        COMMAND
            ${Python3_EXECUTABLE}
            "-B"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_phase6_project_tooling.py"
            "-v"
    )
    set_tests_properties(
        nova-phase6-isolated nova-phase6-project
        PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            TIMEOUT 90
            ENVIRONMENT "NOVA_TEST_EXECUTABLE=$<TARGET_FILE:nova>"
    )

    add_test(
        NAME nova-phase7-runner-unit
        COMMAND
            ${Python3_EXECUTABLE}
            "-B"
            "-m"
            "unittest"
            "tests.test_qualification_runners"
            "-v"
    )
    add_test(
        NAME nova-phase7-qualification
        COMMAND
            ${Python3_EXECUTABLE}
            "-B"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/qualification/run_qualification.py"
            --nova "$<TARGET_FILE:nova>"
            --output "${CMAKE_CURRENT_BINARY_DIR}/qualification"
    )
    set_tests_properties(
        nova-phase7-runner-unit nova-phase7-qualification
        PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            TIMEOUT 300
            LABELS "phase7;qualification"
    )
endif()

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
