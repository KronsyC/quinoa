cmake_minimum_required( VERSION 3.16 )

project( quinoa-compiler )

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER clang++)
execute_process(COMMAND llvm-config --libs OUTPUT_VARIABLE SYS_LIBS )
execute_process(COMMAND llvm-config --system-libs OUTPUT_VARIABLE LIBS )
execute_process(COMMAND llvm-config --ldflags OUTPUT_VARIABLE LDF )
message(STATUS "Found LLVM" ${LIBS})

string(STRIP ${LIBS} LIBS)
string(STRIP ${SYS_LIBS} SYS_LIBS)
string(STRIP ${LDF} LDF)

link_libraries(${LIBS} ${SYS_LIBS} ${LDF})

file(GLOB_RECURSE sources quinoa/*.cc quinoa/*.h quinoa/*.cpp quinoa/*.hh)

add_executable(quinoa ${sources})


# Generate Macro Definitions
execute_process(COMMAND ./scripts/generateTokenDefinitions.py)

target_compile_definitions(quinoa PUBLIC "DEFAGE=int age = 16\; age+=1")
