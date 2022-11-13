# Quinoa

## The Purely Compositional Language

---

## Disclaimer:

### This project only works on x86-64 linux based systems
---

## Dependencies
- CMake
- LLVM (llvm-devel)
- Clang
- Ninja

## How To Build:

1. Clone the repo
    ```bash
    git clone https://github.com/CaseyAllen/quinoa.git
    cd quinoa
    ```
2. Set the `LIBQ_DIR` declaration in `CMakeLists.txt` to `QUINOA_DIR/libq`

3. Build the project
    ```bash
    ./scripts/compile.sh
    ```
4. Compile a sample file
    ```
    ./build/quinoa build ./examples/HelloWorld.qn
    ```

### How to run the output file (*nix):

The quinoa compiler outputs a file containing llvm ir, llvm ir can be thought of as a multiplatform assembly language.

To execute this file, either: 

1. Run the file directly using the llvm ir interpreter,  `lli`
    ```bash
    lli path_to_output.ll
    ```
**OR**

2. Compile the file, then running the resulting executable.
    ```bash
    clang path_to_output.ll
    ```
    then running the resulting binary
    ```bash
    ./a.out
    ```
