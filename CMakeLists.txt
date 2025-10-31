cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(MyANTLRExample LANGUAGES C CXX)

# -------------------------------------------------------------  --------------
# 🧱 Basic Settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_definitions(-DANTLR4CPP_STATIC)

# ---------------------------------------------------------------------------
# 🧩 Build type and debug settings
# ---------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the build type" FORCE)
endif()

# Ensure debug symbols are generated
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

# ---------------------------------------------------------------------------
# ⚙️ External Modules
# ---------------------------------------------------------------------------
# Add our custom cmake modules (for ANTLR integration)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ExternalAntlr4Cpp)

set(ANTLR4_BUILD_TESTS OFF CACHE BOOL "Disable ANTLR4 tests")

# ---------------------------------------------------------------------------
# 🐜 ANTLR Configuration
# ---------------------------------------------------------------------------
set(ANTLR_EXECUTABLE "${CMAKE_SOURCE_DIR}/thirdparty/antlr/antlr-4.13.2-complete.jar")
set(ANTLR4CPP_JAR_LOCATION "${ANTLR_EXECUTABLE}")

find_package(ANTLR REQUIRED)

antlr_target(Expr
    ${CMAKE_CURRENT_SOURCE_DIR}/grammar/Expr.g4
    VISITOR
    COMBINED
)

include_directories(
    "${ANTLR_Expr_OUTPUT_DIR}"
    "${ANTLR4_INCLUDE_DIRS}"
)

# ---------------------------------------------------------------------------
# 🧩 LLVM Configuration
# ---------------------------------------------------------------------------
# Optional: disable ZSTD dependency if LLVM was built with it
set(LLVM_ENABLE_ZSTD OFF CACHE BOOL "Disable ZSTD support in LLVM")

# Find LLVM automatically
find_package(LLVM REQUIRED CONFIG HINTS
    /usr/lib/llvm*/lib/cmake/llvm
    /usr/local/opt/llvm/lib/cmake/llvm
    /opt/homebrew/opt/llvm/lib/cmake/llvm
)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(LLVM_LIBS
    core
    support
    native
    orcjit        # Optional: JIT compilation support
)

# ---------------------------------------------------------------------------
# 🏗️ Build Target
# ---------------------------------------------------------------------------
add_executable(ACCUL
    src/CodegenVisitor.cpp
    src/main.cpp
    ${ANTLR_Expr_CXX_OUTPUTS}
)

target_link_libraries(ACCUL
    PRIVATE
    antlr4_static
    ${LLVM_LIBS}
)

# ---------------------------------------------------------------------------
# 🧪 Enable Testing
# ---------------------------------------------------------------------------
include(CTest)
enable_testing()

# ---------------------------------------------------------------------------
# 🧪 Test Executable
# ---------------------------------------------------------------------------
add_executable(tests
    tests/test.cpp
    src/CodegenVisitor.cpp
    ${ANTLR_Expr_CXX_OUTPUTS}
)

target_include_directories(tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${ANTLR_Expr_OUTPUT_DIR}
    ${ANTLR4_INCLUDE_DIRS}
)

target_link_libraries(tests
    PRIVATE
    antlr4_static
    ${LLVM_LIBS}
)

# ---------------------------------------------------------------------------
# 🧩 Register CTest tests
# ---------------------------------------------------------------------------

# Binary operator test: 4*(2+3)-5 = 15
add_test(NAME BinaryOp
         COMMAND tests "4*(2+3)-5" 15
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Unary operator test: -(3+2)*2 = -10
add_test(NAME UnaryOp
         COMMAND tests "(1 + 2) * -3" -9
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})


# ---------------------------------------------------------------------------
# ✅ Summary
# ---------------------------------------------------------------------------
message(STATUS "")
message(STATUS "================ Build Summary ================")
message(STATUS "C++ Standard     : ${CMAKE_CXX_STANDARD}")
message(STATUS "ANTLR JAR        : ${ANTLR_EXECUTABLE}")
message(STATUS "LLVM Version     : ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM Directory   : ${LLVM_DIR}")
message(STATUS "================================================")
message(STATUS "")
