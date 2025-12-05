# macOS LLVM-only toolchain file
# This file is loaded BEFORE CMake's platform initialization
# Prevents system SDK paths from being added

set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR arm64)

# Compiler paths (will be set from workflow)
set(CMAKE_C_COMPILER "${LLVM_PREFIX}/bin/clang" CACHE FILEPATH "C compiler")
set(CMAKE_CXX_COMPILER "${LLVM_PREFIX}/bin/clang++" CACHE FILEPATH "C++ compiler")

# Completely disable system root
set(CMAKE_OSX_SYSROOT "" CACHE PATH "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "" CACHE STRING "")

# Block all implicit include directories
set(CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES "" CACHE STRING "")
set(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES "" CACHE STRING "")
set(CMAKE_PLATFORM_IMPLICIT_INCLUDE_DIRECTORIES "" CACHE STRING "")

# Disable framework search
set(CMAKE_FIND_FRAMEWORK NEVER CACHE STRING "")
set(CMAKE_FIND_APPBUNDLE NEVER CACHE STRING "")

# Force no system include paths
set(CMAKE_SYSTEM_INCLUDE_PATH "" CACHE STRING "")
set(CMAKE_SYSTEM_PREFIX_PATH "" CACHE STRING "")

# Explicitly set LLVM include paths (will be set from workflow)
set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES
    "${LLVM_PREFIX}/lib/clang/${CLANG_VERSION}/include"
    CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES
    "${LLVM_PREFIX}/include/c++/v1"
    "${LLVM_PREFIX}/lib/clang/${CLANG_VERSION}/include"
    CACHE STRING "" FORCE)

# Skip compiler checks that might add system paths
set(CMAKE_C_COMPILER_WORKS TRUE CACHE BOOL "")
set(CMAKE_CXX_COMPILER_WORKS TRUE CACHE BOOL "")

# Aggressive compiler flags to block system headers
set(CMAKE_C_FLAGS_INIT "-nostdinc -nostdlibinc" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_INIT "-nostdinc++ -nostdinc -nostdlibinc" CACHE STRING "" FORCE)
