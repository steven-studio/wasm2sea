#include "wasm_instr.hpp"
#include "wasm_lower.hpp"
#include "value_ir_dump.hpp"
#include "ir_bridge.hpp"
#include "wasm_reader.hpp"
#include "wasm_dump.hpp"
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

    // ✅ 读取所有函数
    std::vector<FunctionResult> functions;

    if (argc > 1) {
        functions = readWasmFile(argv[1]);
        if (functions.empty()) {
            std::cerr << "Failed to read WASM file or no functions found\n";
            return 1;
        }
        
        std::cout << "Loaded " << functions.size() << " function(s)\n\n";
    } else {
        std::cout << "No input WASM file provided\n";
        return 0;
    }

    // ✅ 处理所有函数
    for (size_t i = 0; i < functions.size(); i++) {
        const auto& func = functions[i];

        std::cout << "\n" << std::string(70, '=') << "\n";
        std::cout << "Processing function [" << i << "]: " << func.name << "\n";
        std::cout << "Parameters: " << func.numParams << "\n";
        std::cout << std::string(70, '=') << "\n\n";

        InstrSeq code = func.instructions;

        // Step 1: Wasm → ValueIR (你的 SSA IR)
        std::cout << "Step 1: Lowering Wasm to ValueIR\n";
        std::cout << "================================\n";

        // ===== 添加这里 =====
        printf("\n");
        dumpInstrSeq(code);  // 打印 Stage 1 输出
        printf("\n");
        // ===================

        ValueIR values = lowerWasmToSsa(code);
        dumpValueIR(values);

        // Step 2: ValueIR → dstogov/ir
        std::cout << "\nStep 2: Building dstogov/ir Graph\n";
        std::cout << "==================================\n";

        IRBridge bridge;
        IRFunction* fn = bridge.build(values);
        
        // 印出 dstogov/ir 的 IR graph
        bridge.dump(fn);

        // 保存每个函数的 IR
        std::string irPath = func.name + ".ir";
        bridge.save(irPath.c_str());
        std::cout << "Saved IR to: " << irPath << "\n";

        // 清理
        delete fn;
    }
    
    // ✅ 移到这里：循环外
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "=== Compilation Complete ===\n";
    std::cout << "Successfully processed " << functions.size() << " function(s)\n";
    std::cout << std::string(70, '=') << "\n";
    
    return 0;
}
