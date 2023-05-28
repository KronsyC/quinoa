# Quinoa

## An experimental programming language

---

## This project is now deprecated, it served as a wonderful C++ learning experience for me, but I am now working on other stuff.

---

## Dependencies
- CMake
- LLVM (llvm-devel) version 15
- Clang

## How To Build:

1. Clone the repo
    ```bash
    git clone https://github.com/KronsyC/quinoa.git
    cd quinoa
    ```
2. Build the project
    ```bash
    cmake . -B build
    cmake --build build/
    ```
3. Compile a sample file
   NOTE: The current version of the compiler uses a hacky import system dependant
   on the build_includes script, and a json packaging format. This WILL change in later
   versions
    ```
    ./build/quinoa build ./examples/HelloWorld.qn -o helloworld -i $(./build_includes ./libq/qpk.json)
    ```
