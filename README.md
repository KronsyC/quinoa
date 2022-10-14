# Quinoa

## The Purely Compositional Language

---

## Disclaimer:

### This project only works on x86-64 linux based systems
---

## Dependencies
- CMake
- LLVM
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
````
4. Compile a sample file
```
./build/quinoa build ./examples/HelloWorld.qn
```