# Wasm2Sea Compiler

A WebAssembly to native code compiler using Binaryen and dstogov/ir.

## Overview

Wasm2Sea translates WebAssembly bytecode to an intermediate representation (IR), then generates native code through the dstogov/ir backend. The project demonstrates a complete compilation pipeline from WASM to executable code.

## Features

### Implemented ✓
- **Arithmetic Operations**: add, sub, mul, div (signed/unsigned), rem
- **Comparisons**: eq, ne, lt, gt, le, ge (signed/unsigned), eqz
- **Bitwise Operations**: and, or, xor, shl, shr (arithmetic/logical)
- **Local Variables**: local.get, local.set, local.tee
- **Control Flow**: 
  - Select (ternary operator)
  - If-else with complex branches
  - Nested conditionals

### In Progress 🚧
- **Loops**: Loop structures parsed, execution not yet implemented

### Planned 📋
- Block structures
- Function calls
- Memory operations
- Additional types (i64, f32, f64)

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

This is a research/educational project demonstrating WebAssembly compilation. Current focus is on implementing loop support, which is the most complex remaining feature.

See [PROGRESS.md](PROGRESS.md) for detailed status and [TODO.md](TODO.md) for roadmap.

## License

[Add your license here]

## References

- [Binaryen](https://github.com/WebAssembly/binaryen)
- [dstogov/ir](https://github.com/dstogov/ir)
- [WebAssembly Specification](https://webassembly.github.io/spec/)
