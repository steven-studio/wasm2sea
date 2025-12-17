# Wasm2Sea TODO List

## High Priority 🔴

### Loop Implementation (Estimated: 2-3 days)
- [ ] Implement loop control flow in wasm_lower
- [ ] Generate Phi nodes for loop variables
- [ ] Handle back edges (br_if jumping back to loop start)
- [ ] Generate proper CFG in ir_bridge
  - [ ] ir_LOOP_BEGIN / ir_LOOP_END
  - [ ] ir_IJMP for backward jumps
- [ ] Test cases:
  - [ ] Simple countdown loop
  - [ ] Loop with accumulation (sum)
  - [ ] Nested loops

### Block Structures
- [ ] Parse block with labels
- [ ] Implement br (unconditional break)
- [ ] Support early exit from blocks
- [ ] Test with complex control flow

## Medium Priority 🟡

### Void-type If
- [ ] Handle if statements with side effects only
- [ ] Proper control flow without return values
- [ ] Update ControlFrame tracking

### Function Calls
- [ ] Parse call instructions
- [ ] Handle multiple functions in module
- [ ] Function signature management
- [ ] Recursive function support

### Memory Operations
- [ ] i32.load / i32.store
- [ ] Memory size management
- [ ] Bounds checking

## Low Priority 🟢

### Type System
- [ ] i64 support
- [ ] f32 / f64 floating point
- [ ] Type conversions

### Advanced Control Flow
- [ ] br_table (switch-case)
- [ ] Indirect calls (call_indirect)

### Optimizations
- [ ] Dead code elimination
- [ ] Constant folding
- [ ] Loop invariant code motion

## Known Issues 🐛

### Control Flow
- Void-type if generates incorrect IR (returns constant instead of using control flow)
- Loop and br_if parsed but not lowered to IR
- No support for forward branches yet

### Code Quality
- ExpressionStackWalker replaced with Visitor (refactoring complete)
- Need to add more comprehensive tests
- IR dump contains debug output that should be removable

### Documentation
- Need to document IR conventions
- Add examples for each supported feature
- Document dstogov/ir quirks (GT→LT transformation)

## Future Enhancements 💡

- Source maps for debugging
- Better error messages
- WASI support
- WebAssembly SIMD
- Multi-value returns
- Exception handling

## Notes

- dstogov/ir optimizations affect PHI ordering (GT becomes LT with swapped operands)
- Select and if-else require different PHI parameter orders due to this
- Current focus: Get loops working before moving to other features
