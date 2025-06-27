cmake_minimum_required(VERSION 3.7)

if(POLICY CMP0114)
  cmake_policy(SET CMP0114 NEW)
endif()

include(ExternalProject)

# Paths
set(ANTLR4_ZIP_REPOSITORY "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/antlr/antlr4-master.zip")
set(ANTLR4_ROOT ${CMAKE_CURRENT_BINARY_DIR}/antlr4_runtime/src/antlr4_runtime)
set(ANTLR4_INCLUDE_DIRS ${ANTLR4_ROOT}/runtime/Cpp/runtime/src)

file(MAKE_DIRECTORY "${ANTLR4_INCLUDE_DIRS}")

# Build dirs and libs setup unchanged
if(MSVC)
  set(ANTLR4_STATIC_LIBRARIES ${ANTLR4_ROOT}/runtime/Cpp/debug/antlr4-runtime-static.lib)
else()
  set(ANTLR4_STATIC_LIBRARIES ${ANTLR4_ROOT}/runtime/Cpp/runtime/libantlr4-runtime.a)
endif()

# Use zip-based source instead of git
ExternalProject_Add(
  antlr4_runtime
  PREFIX antlr4_runtime
  URL "${ANTLR4_ZIP_REPOSITORY}"
  DOWNLOAD_DIR "${CMAKE_CURRENT_BINARY_DIR}"
  BUILD_IN_SOURCE 1
  SOURCE_DIR "${ANTLR4_ROOT}"
  SOURCE_SUBDIR runtime/Cpp
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DFORCE_STATIC_CRT=ON
    -DDISABLE_WARNINGS=ON
  BUILD_COMMAND ""  # build manually via steps
  INSTALL_COMMAND ""
  EXCLUDE_FROM_ALL 1
)

# Build step
ExternalProject_Add_Step(
  antlr4_runtime build_static
  COMMAND ${CMAKE_COMMAND} --build . --target antlr4_static
  WORKING_DIRECTORY ${ANTLR4_ROOT}/runtime/Cpp
  DEPENDS antlr4_runtime
  BYPRODUCTS ${ANTLR4_STATIC_LIBRARIES}
  EXCLUDE_FROM_MAIN 1
)
ExternalProject_Add_StepTargets(antlr4_runtime build_static)

add_library(antlr4_static STATIC IMPORTED)
add_dependencies(antlr4_static antlr4_runtime-build_static)
set_target_properties(antlr4_static PROPERTIES
  IMPORTED_LOCATION ${ANTLR4_STATIC_LIBRARIES}
)
target_include_directories(antlr4_static INTERFACE ${ANTLR4_INCLUDE_DIRS})
