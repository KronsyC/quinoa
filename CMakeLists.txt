cmake_minimum_required( VERSION 3.24 )

project( quinoa-compiler )

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM " ${LLVM_PACKAGE_VERSION})

include_directories(${LLVM_INCLUDE_DIRS} )

add_definitions(${LLVM_DEFINITIONS})

set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if(DEBUG)
    message(STATUS "Debug Mode Active")
    add_compile_options(-DDEBUG=1)
endif()

add_compile_options(-fdiagnostics-color=always)
add_compile_options(-O0)

llvm_map_components_to_libnames(llvm_libs core linker x86 target support)

message(STATUS "Linking LLVM libraries: ${llvm_libs}")

add_definitions(-DQUINOA_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
file(GLOB_RECURSE sources quinoa/*.cc quinoa/*.h quinoa/*.cpp quinoa/*.hh)



add_executable(quinoa ${sources})
target_link_libraries(quinoa PRIVATE LLVM)
target_compile_options(quinoa PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-missing-field-initializers -Wno-ambiguous-reversed-operator)

