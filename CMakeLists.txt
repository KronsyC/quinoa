cmake_minimum_required( VERSION 3.24 )

project( quinoa-compiler )

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM " ${LLVM_PACKAGE_VERSION})

include_directories(${LLVM_INCLIDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_BUILD_TYPE DEBUG)

if(DEBUG)
    message(STATUS "Debug Build Active")
    add_compile_options(-DDEBUG=1)
endif()
add_compile_options(-fdiagnostics-color=always)
add_compile_options(-O1)

llvm_map_components_to_libnames(llvm_libs support core irreader linker mcjit native)

add_definitions(-DQUINOA_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
file(GLOB_RECURSE sources quinoa/*.cc quinoa/*.h quinoa/*.cpp quinoa/*.hh)
add_executable(quinoa ${sources} quinoa/core/Parser/parser_utils.hh quinoa/core/Preprocessor/passes/required/type_ref_resolver.hh quinoa/core/AST/literal.hh)
target_link_libraries(quinoa ${llvm_libs})
target_compile_options(quinoa PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-missing-field-initializers -g)

