#include "wasm_instr.hpp"
#include "wasm_lower.hpp"
#include "value_ir_dump.hpp"
#include "ir_bridge.hpp"
#include "wasm_reader.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "=== Wasm2Sea Compiler Pipeline ===\n\n";
    
    InstrSeq code;

    // Step 0: 準備 Wasm 指令序列
    // (func (param $x i32) (param $y i32) (result i32)
    //   local.get 0
    //   local.get 1
    //   i32.add
    //   return
    // )
    if (argc > 1) {
        code = readWasmFile(argv[1]);
        if (code.empty()) {
            std::cerr << "Failed to read WASM file\n";
            return 1;
        }
    } else {
        // 預設範例
        InstrSeq code = {
            {WasmOp::LocalGet, 0},
            {WasmOp::LocalGet, 1},
            {WasmOp::I32Add, 0},
            {WasmOp::Return, 0}
        };
    }

    // Step 1: Wasm → ValueIR (你的 SSA IR)
    std::cout << "Step 1: Lowering Wasm to ValueIR\n";
    std::cout << "================================\n";
    ValueIR values = lowerWasmToSsa(code);
    dumpValueIR(values);

    // Step 2: ValueIR → dstogov/ir
    std::cout << "\nStep 2: Building dstogov/ir Graph\n";
    std::cout << "==================================\n";
    IRBridge bridge;
    IRFunction* fn = bridge.build(values);
    
    // 印出 dstogov/ir 的 IR graph
    bridge.dump(fn);

    // 清理
    delete fn;
    
    std::cout << "\n=== Compilation Complete ===\n";
    return 0;
}
