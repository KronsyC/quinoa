cmake_minimum_required( VERSION 3.24 )

project( quinoa-compiler )

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM " ${LLVM_PACKAGE_VERSION})

include_directories(${LLVM_INCLIDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_BUILD_TYPE DEBUG)


add_compile_options(-O1)

llvm_map_components_to_libnames(llvm_libs support core irreader linker mcjit native)

add_definitions(-DQUINOA_DIR="${CMAKE_CURRENT_SOURCE_DIR}" -DDEBUG_MODE=1 -freport-bug)
file(GLOB_RECURSE sources quinoa/*.cc quinoa/*.h quinoa/*.cpp quinoa/*.hh)
add_executable(quinoa ${sources})
target_link_libraries(quinoa ${llvm_libs})
target_compile_options(quinoa PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-missing-field-initializers -g)

