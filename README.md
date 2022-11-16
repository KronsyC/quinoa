# Quinoa

## An experimental programming language

---

## Disclaimer:

### This project only works on x86-64 linux based systems
---

## Dependencies
- CMake
- LLVM (llvm-devel)
- Clang

## How To Build:

1. Clone the repo
    ```bash
    git clone https://github.com/CaseyAllen/quinoa.git
    cd quinoa
    ```
2. Build the project
    ```bash
    cmake . -B build
    cmake --build build/
    ```
3. Compile a sample file
    ```
    ./build/quinoa build ./examples/HelloWorld.qn -o hello_world
    ```
