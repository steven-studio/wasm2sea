#include "wasm_instr.hpp"
#include "wasm_lower.hpp"
#include "value_ir_dump.hpp"
#include "ir_bridge.hpp"
#include "wasm_reader.hpp"
#include <iostream>
#include <string>

static void usage(const char* prog) {
    std::cerr
        << "Usage:\n"
        << "  " << prog << " <input.wasm> [--save-ir <out.ir>]\n"
        << "  " << prog << " (no args: uses built-in example)\n";
}

int main(int argc, char* argv[]) {
    std::cout << "=== Wasm2Sea Compiler Pipeline ===\n\n";
    
    std::string wasmPath;
    std::string saveIrPath;

    // ---- argv parsing (minimal) ----
    // First non-flag arg is input.wasm
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--help" || a == "-h") {
            usage(argv[0]);
            return 0;
        } else if (a == "--save-ir") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --save-ir requires a path\n";
                return 2;
            }
            saveIrPath = argv[++i];
        } else if (!a.empty() && a[0] == '-') {
            std::cerr << "Unknown option: " << a << "\n";
            usage(argv[0]);
            return 2;
        } else {
            wasmPath = a;
        }
    }

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
        std::cout << "No input WASM file provided\n";
        return 0;
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

    // ★ 核心驗證輸出
    bridge.save("out.ir");

    // 清理
    delete fn;
    
    std::cout << "\n=== Compilation Complete ===\n";
    return 0;
}
