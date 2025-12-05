# macOS LLVM-only toolchain file
# This file is loaded BEFORE CMake's platform initialization
# Prevents system SDK paths from being added

set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR arm64)

# Compiler paths (will be set from workflow)
set(CMAKE_C_COMPILER "${LLVM_PREFIX}/bin/clang" CACHE FILEPATH "C compiler")
set(CMAKE_CXX_COMPILER "${LLVM_PREFIX}/bin/clang++" CACHE FILEPATH "C++ compiler")

# Completely disable system root
set(CMAKE_OSX_SYSROOT "" CACHE PATH "" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET "" CACHE STRING "" FORCE)

# Block all implicit include directories
set(CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES "" CACHE STRING "" FORCE)
set(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES "" CACHE STRING "" FORCE)
set(CMAKE_PLATFORM_IMPLICIT_INCLUDE_DIRECTORIES "" CACHE STRING "" FORCE)

# Disable framework search
set(CMAKE_FIND_FRAMEWORK NEVER CACHE STRING "" FORCE)
set(CMAKE_FIND_APPBUNDLE NEVER CACHE STRING "" FORCE)

# Force no system include paths
set(CMAKE_SYSTEM_INCLUDE_PATH "" CACHE STRING "" FORCE)
set(CMAKE_SYSTEM_PREFIX_PATH "" CACHE STRING "" FORCE)

# Skip compiler checks that might add system paths
set(CMAKE_C_COMPILER_WORKS TRUE CACHE BOOL "" FORCE)
set(CMAKE_CXX_COMPILER_WORKS TRUE CACHE BOOL "" FORCE)

# CRITICAL: Use -I (high priority) instead of -isystem (low priority)
# Add LLVM headers FIRST with -I so they override any -isystem paths
set(CMAKE_C_FLAGS_INIT "-nostdinc -nostdlibinc -I${LLVM_PREFIX}/lib/clang/${CLANG_VERSION}/include --sysroot=" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_INIT "-nostdinc++ -nostdinc -nostdlibinc -I${LLVM_PREFIX}/include/c++/v1 -I${LLVM_PREFIX}/lib/clang/${CLANG_VERSION}/include --sysroot=" CACHE STRING "" FORCE)
