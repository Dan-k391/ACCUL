cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
project(MyANTLRExample LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_definitions(-DANTLR4CPP_STATIC)

# Add our cmake modules to path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(ExternalAntlr4Cpp)

set(ANTLR4_BUILD_TESTS OFF CACHE BOOL "Disable ANTLR4 tests")

set(ANTLR_EXECUTABLE "${CMAKE_SOURCE_DIR}/thirdparty/antlr/antlr-4.13.2-complete.jar")
set(ANTLR4CPP_JAR_LOCATION "${ANTLR_EXECUTABLE}")

set(LLVM_DIR "/usr/lib/llvm-14/lib/cmake/llvm")

find_package(ANTLR REQUIRED)

antlr_target(Expr
    ${CMAKE_CURRENT_SOURCE_DIR}/grammar/Expr.g4
    COMBINED
)

include_directories(
    "${ANTLR_Expr_OUTPUT_DIR}"
    "${ANTLR4_INCLUDE_DIRS}"
)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(LLVM_LIBS
    core
    support
    native
    orcjit       # Optional: for JIT support
)

add_executable(myparser
    src/main.cpp
    ${ANTLR_Expr_CXX_OUTPUTS}
)

# Link both antlr and llvm
target_link_libraries(myparser PRIVATE antlr4_static ${LLVM_LIBS})
