cmake_minimum_required(VERSION 3.20)

# Examples are optional and don't need to build during main compilation
# They will be used with the nova compiler after it's built

message(STATUS "Examples directory configured")

# Just mark that examples exist
# Examples will be compiled using nova compiler itself after build:
# ./build/Release/nova compile examples/hello.ts
# ./build/Release/nova compile examples/fibonacci.ts
# ./build/Release/nova compile examples/async.ts

# No build targets needed here
