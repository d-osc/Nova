cmake_minimum_required(VERSION 3.20)

# Enable testing
enable_testing()

# Find GTest (optional, can use custom test framework)
find_package(GTest)

if(NOT GTEST_FOUND)
    message(STATUS "GTest not found, downloading...")
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
    )
    FetchContent_MakeAvailable(googletest)
endif()

# Test sources
set(TEST_SOURCES
    unit/LexerTest.cpp
    unit/ParserTest.cpp
    unit/TypeCheckerTest.cpp
    unit/HIRGenTest.cpp
    unit/MIRGenTest.cpp
    unit/CodeGenTest.cpp
    integration/CompilationTest.cpp
    integration/OptimizationTest.cpp
)

# Create test executable
add_executable(nova_tests ${TEST_SOURCES})

target_link_libraries(nova_tests
    PRIVATE
    novacore
    GTest::gtest_main
    ${llvm_libs}
)

target_include_directories(nova_tests
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

# Add tests
include(GoogleTest)
gtest_discover_tests(nova_tests)

# Add custom test targets
add_custom_target(test-lexer
    COMMAND ${CMAKE_CTEST_COMMAND} -R Lexer --output-on-failure
    DEPENDS nova_tests
)

add_custom_target(test-parser
    COMMAND ${CMAKE_CTEST_COMMAND} -R Parser --output-on-failure
    DEPENDS nova_tests
)

add_custom_target(test-all
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS nova_tests
)
