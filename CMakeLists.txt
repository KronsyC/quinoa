cmake_minimum_required( VERSION 3.24 )

project( quinoa-compiler )

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_BUILD_TYPE DEBUG)


execute_process(COMMAND llvm-config --libs OUTPUT_VARIABLE SYS_LIBS )
execute_process(COMMAND llvm-config --system-libs OUTPUT_VARIABLE LIBS )
execute_process(COMMAND llvm-config --ldflags OUTPUT_VARIABLE LDF )
message(STATUS "Found LLVM" ${LIBS})

string(STRIP ${LIBS} LIBS)
string(STRIP ${SYS_LIBS} SYS_LIBS)
string(STRIP ${LDF} LDF)

link_libraries(${LIBS} ${SYS_LIBS} ${LDF})

add_compile_options(-O1)


add_definitions(-DLIBQ_DIR="/home/casey/quinoa/libq")
file(GLOB_RECURSE sources quinoa/*.cc quinoa/*.h quinoa/*.cpp quinoa/*.hh)
# include(FetchContent)
# FetchContent_Declare(toml11                              # name of the content
  # GIT_REPOSITORY https://github.com/ToruNiina/toml11.git  # the repository
  # GIT_TAG        v3.7.1
# )
# FetchContent_MakeAvailable(toml11)
add_executable(quinoa ${sources})
target_compile_options(quinoa PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter)

# target_link_libraries(quinoa PRIVATE toml11)
