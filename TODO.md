# Wasm2Sea TODO List

## Verified ✓✓ (Differential Testing)

### PolyBenchC — 27/29 kernels passing
Stencils: jacobi-1d, jacobi-2d, seidel-2d, heat-3d, fdtd-2d, adi
Linear Algebra: gemm, 2mm, 3mm, atax, bicg, gemver, gesummv, symm, syrk, syr2k, trmm, mvt
Solvers: trisolv, durbin, lu, ludcmp, cholesky, gramschmidt
Datamining: covariance, correlation
Medley: floyd-warshall

All verified against reference C implementations using `run_polybenchc.sh`.
Remaining: deriche, nussinov (Medley category).

## Completed ✓

### Control Flow
- [x] Loop control flow in wasm_lower (Phi nodes for loop variables, back edges)
- [x] `ir_LOOP_BEGIN` / `ir_LOOP_END` generation in ir_bridge
- [x] Block structures with labels, `br` (unconditional), `br_if` (conditional)
- [x] Nested loops and nested conditionals
- [x] Void-type if (side-effect-only, no return value)
- [x] Select (ternary operator) via PHI
- [x] Block-scoped branches correctly distinguished from loop-exit checks —
      previously any `br_if` targeting a `Block` was assumed to be a loop-exit
      check; this misclassified block-scoped jumps used to encode
      value-producing ternaries (clang -O0's block+br_if+br idiom) as loop
      exits, silently severing the enclosing loop's backedge and causing
      infinite loops. Fixed by checking whether the innermost control frame
      is actually a `Loop` at the point of the `br_if`.

### Types & Arithmetic
- [x] i32 arithmetic, comparisons, bitwise ops, clz/ctz/popcnt
- [x] i64 basic arithmetic (add/sub/mul/div_s/rem_s), sign/zero extend, wrap
- [x] F64 arithmetic: const, add, sub, mul, div, neg, sqrt
- [x] F64 comparisons: eq, ne, lt, gt, le, ge (previously missing across
      wasm_lower.cpp and ir_bridge.cpp despite being defined in the WasmOp/Op
      enums and correctly parsed from binaryen's AST — silently corrupted the
      value stack via `handle_Unsupported` when hit)
- [x] Type conversions: i32→f64, f64→i32/i64 (trunc), i64 sign/zero extend

### Memory
- [x] i32.load / i32.store
- [x] f64.load / f64.store
- [x] wasm global variable init values correctly read from the module
      (previously defaulted to 0, causing segfaults on kernels with large
      stack frames — e.g. `__stack_pointer`)

### Calls
- [x] Calls to external/libc functions (e.g. `sqrt`) via `ir_CALL_1`

## High Priority 🔴

### User-Defined Function Calls
- [ ] Multiple functions in a single module
- [ ] Function signature management across calls
- [ ] Recursive function support
- [ ] Test with PolyBenchC kernels that factor out helper functions

### Remaining PolyBenchC Coverage
- [ ] Medley category: deriche, nussinov — floyd-warshall completed
      (turned out to need a ternary-expression rewrite, not br_table)

## Medium Priority 🟡

### Advanced Control Flow
- [ ] `br_table` (switch-case) — currently unimplemented; no handler exists
      in wasm_lower.cpp or ir_bridge.cpp for `WasmOp::BrTable`/`Op::BrTable`
- [ ] Indirect calls (`call_indirect`)

### Memory Operations
- [ ] memory.grow
- [ ] memory.copy / memory.fill edge cases (basic support exists via
      external-call encoding, needs broader testing)

## Low Priority 🟢

### Optimizations
- [ ] Dead code elimination beyond current degenerate-PHI cleanup
- [ ] Constant folding
- [ ] Loop invariant code motion

### Type System
- [ ] f32 support (currently only f64)
- [ ] Multi-value returns

## Known Issues 🐛

### Resolved — Ternary-expression / value-merging type bugs (found together)
Three distinct bugs, only surfaced when compiling a genuine value-producing
ternary through the wasm block+br_if idiom for the first time (floyd-warshall
was the first kernel exercising this path with a double-typed merge):
- `lowerWasmToSsa`'s main dispatch loop iterated over the ORIGINAL
  (unrewritten) instruction sequence instead of the ternary-rewritten one,
  silently making a correctly-firing rewrite pass a complete no-op.
- `ir_bridge.cpp`'s `handleLocalGet` only distinguished `IR_I64` vs `IR_I32`
  for `VLOAD`, misreading `IR_DOUBLE`-typed locals as 32-bit integers.
- `ir_bridge.cpp`'s `handlePhi` hardcoded `IR_I32` for if/else value-merging
  Phi nodes regardless of the actual merged value type.
Root-caused via a minimal standalone `.wat` repro isolating the exact
scenario independent of clang/PolyBenchC — essential since the three bugs'
symptoms overlapped and debugging them simultaneously through the full
pipeline was far harder than isolating each with a synthetic test case.

### Code Quality
- Need more comprehensive regression tests beyond PolyBenchC (targeted unit
  tests for individual ops, similar to `run_differential.sh`)
- Some debug trace output (`TRACE(...)`) still present; controlled by
  `ENABLE_IR_TRACE`, currently off by default

### Documentation
- Document dstogov/ir quirks (GT→LT transformation affecting PHI operand
  ordering for Select and if-else)
- Add architecture notes on the three-layer lowering pipeline
  (binaryen AST → WasmOp → ValueIR (Op) → dstogov/ir) and where to add
  support for a new wasm instruction (all three layers must be updated;
  missing any one layer causes silent fallthrough to `handle_Unsupported`
  rather than a compile error — this has been the root cause of every
  correctness bug found so far)

## Future Enhancements 💡

- Source maps for debugging
- Better error messages (currently many silent no-ops on malformed input)
- WASI support
- WebAssembly SIMD
- Exception handling

## Notes

- dstogov/ir optimizations affect PHI ordering (GT becomes LT with swapped
  operands); Select and if-else require different PHI parameter orders as
  a result.
- **Lesson learned**: every correctness bug found during PolyBenchC
  validation (global init values, F64Neg, F64 comparisons, br_if
  misclassification) stemmed from an operation being defined in the
  WasmOp/Op enums and correctly recognized by binaryen's AST converter, but
  missing a handler in one of the two downstream layers (wasm_lower.cpp or
  ir_bridge.cpp). None of these caused a compile error — they either
  silently dropped values or, in the br_if case, silently corrupted control
  flow. When adding support for a new wasm instruction, always verify all
  three layers are wired up, not just the enum definitions.
- Current focus: extend PolyBenchC coverage and add user-defined function
  call support.
