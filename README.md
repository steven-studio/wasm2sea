# Wasm2Sea Compiler

A WebAssembly to native code compiler using Binaryen and dstogov/ir.

## Overview

Wasm2Sea translates WebAssembly bytecode to an intermediate representation (IR), then generates native code through the dstogov/ir backend. The project demonstrates a complete compilation pipeline from WASM to executable code.

## Features

### Implemented ✓
- **Arithmetic Operations**: add, sub, mul, div (signed/unsigned), rem
- **Comparisons**: eq, ne, lt, gt, le, ge (signed/unsigned), eqz
- **Bitwise Operations**: and, or, xor, shl, shr (arithmetic/logical)
- **Unary Operations**: clz, ctz, popcnt
- **Local Variables**: local.get, local.set, local.tee
- **Floating Point (F64)**: const, add, sub, mul, div, abs, neg, sqrt, min, max, eq, ne, lt, gt, le, ge, convert (i32→f64), trunc (f64→i32)
- **Memory**: i32.load, i32.store, f64.load, f64.store
- **Control Flow**: 
  - Select (ternary operator)
  - If-else with complex branches
  - Nested conditionals
  - Loop with br, br_if
  - Block-scoped branches correctly distinguished from loop-exit checks
    (e.g. clang -O0's block+br_if+br encoding of value-producing ternaries)

### Verified via Differential Testing ✓✓
- **PolyBenchC**: 28/29 kernels passing against reference C implementations,
  spanning Stencils, Linear Algebra, Solvers, Datamining, and Medley
  categories: jacobi-1d, jacobi-2d, seidel-2d, heat-3d, fdtd-2d, adi, gemm,
  2mm, 3mm, atax, bicg, gemver, gesummv, symm, syrk, syr2k, trmm, mvt,
  trisolv, durbin, lu, ludcmp, cholesky, gramschmidt, covariance,
  correlation, floyd-warshall, nussinov
- **Ternary-expression correctness**: value-producing ternaries compiled by
  clang -O0 via the block+block+eqz+br_if(0)+localset+br(1) idiom (e.g.
  `path[i][j] < sum ? path[i][j] : sum` in floyd-warshall) are detected by
  a pre-pass and rewritten into the existing, well-tested If/Else/End
  representation, rather than trying to generalize Block/Br_if handling

### Planned 📋
- User-defined function calls
- Additional memory operations (memory.grow, memory.copy edge cases)

See [TODO.md](TODO.md) for complete list.

## Building

### Prerequisites
- CMake 3.10+
- C++17 compiler
- Binaryen library
- dstogov/ir framework

### Compile
```bash
mkdir build && cd build
cmake ..
make
```

## Usage

### Basic Workflow

1. **Write WebAssembly text format (.wat)**:
```wasm
(module
  (func $add (export "test") (param i32 i32) (result i32)
    local.get 0
    local.get 1
    i32.add
  )
)
```

2. **Compile to WASM**:
```bash
wat2wasm add.wat -o add.wasm
```

3. **Convert to IR**:
```bash
./wasm2sea add.wasm
```

4. **Generate and compile C code**:
```bash
cd ../third_party/dstogov-ir
./ir_main ../../build/out.ir --emit-c out.c
cc -O2 out.c run_varargs_ffi.c -o a.out $(pkg-config --cflags --libs libffi)
```

5. **Run**:
```bash
./a.out 10 20  # Returns 30
```

### Testing Script

Use the provided test script:
```bash
./test_wasm_ffi.sh test_add 10 20
```

## Architecture

### Pipeline Stages

1. **wasm_reader**: Parse WASM binary using Binaryen
2. **wasm_lower**: Lower to SSA-form ValueIR
3. **ir_bridge**: Convert to dstogov/ir graph
4. **dstogov/ir**: Generate native code

### Key Design Decisions

- **Visitor Pattern**: Manual control over AST traversal for correct control flow handling
- **SSA Form**: Each value assigned once, simplifies optimization
- **Label Management**: WebAssembly labels converted to depth-based indexing
- **PHI Nodes**: Handle value merging at control flow joins

### Known Quirks

- dstogov/ir converts `GT(a,b)` to `LT(b,a)` internally
- This affects PHI parameter ordering in control flow
- Select and if-else require different PHI orders as a result
- `br_if` targeting a `Block` must only be treated as a loop-exit check when
  the innermost control frame is a `Loop`; misclassifying block-scoped
  branches (e.g. ternary expressions encoded as block+br_if+br at -O0) as
  loop-exits silently corrupts the enclosing loop's control flow (fixed;
  see commit history for the correlation.c PolyBenchC kernel)

## Examples

See [tests/](tests/) directory for examples:
- `test_select.wat`: Ternary operator
- `test_if_else.wat`: Conditional branches  
- `test_if_complex.wat`: Complex branch logic
- `test_loop_countdown.wat`: Loop structure (parsing only)

## Development

### Running Tests
```bash
# Individual test
./test_wasm_ffi.sh test_name [args...]

# Example
./test_wasm_ffi.sh test_if_else 10 5  # Expected: 10
```

### Debug Output
Uncomment debug prints in wasm_reader.cpp to see instruction sequence.

### Adding New Operations

1. Add to `WasmOp` enum in `wasm_instr.hpp`
2. Handle in `visitExpression` in `wasm_reader.cpp`
3. Lower to ValueIR in `wasm_lower.cpp`
4. Map to dstogov/ir in `ir_bridge.cpp`

## Project Status

This is a research/educational project demonstrating WebAssembly compilation.
The compiler has been validated against 21/21 PolyBenchC kernels using
differential testing against reference C implementations. Current focus is
on extending coverage to remaining PolyBenchC categories (Medley: deriche,
floyd-warshall, nussinov) and user-defined function calls.

See [PROGRESS.md](PROGRESS.md) for detailed status and [TODO.md](TODO.md) for roadmap.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## References

- [Binaryen](https://github.com/WebAssembly/binaryen)
- [dstogov/ir](https://github.com/dstogov/ir)
- [WebAssembly Specification](https://webassembly.github.io/spec/)
