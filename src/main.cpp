#include "wasm_instr.hpp"
#include "wasm_lower.hpp"
#include "value_ir_dump.hpp"
#include "ir_bridge.hpp"
#include "wasm_reader.hpp"
#include "wasm_dump.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <regex>
#include <set>

extern "C" {
#include "ir.h"

}

static void usage(const char* prog) {
    std::cerr
        << "Usage:\n"
        << "  " << prog << " <input.wasm> [--save-ir <out.ir>]\n"
        << "  " << prog << " (no args: uses built-in example)\n";
}

static uint8_t wasm_memory[65536] = {0};

// 把不合法的 C 識別符轉成合法的（e.g. "0" -> "func_0"）
static std::string sanitize_cname(const std::string& name) {
    if (name.empty()) return "func_unknown";
    if (std::isdigit((unsigned char)name[0])) return "func_" + name;
    return name;
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

    std::string baseName = wasmPath;
    size_t slash = baseName.find_last_of("/\\");
    if (slash != std::string::npos) baseName = baseName.substr(slash + 1);
    size_t dot = baseName.find_last_of('.');
    if (dot != std::string::npos) baseName = baseName.substr(0, dot);
    std::string irPath = baseName + ".ir";
    FILE* irFile = fopen(irPath.c_str(), "w");
    std::string cPath = std::string(getenv("HOME")) + "/wasm2sea/third_party/dstogov-ir/out.c";
    FILE* cFile = fopen(cPath.c_str(), "w");
    // header will be written after processing all functions
    fclose(cFile);

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

        // debug dump disabled for benchmark mode
        // dumpInstrSeq(code);

        // 建函數名稱表
        std::vector<std::string> funcNames;
        for (const auto& f : functions)
            funcNames.push_back(f.name);

        ValueIR values = lowerWasmToSsa(code, funcNames);
        dumpValueIR(values);

        // Step 2: ValueIR → dstogov/ir
        std::cout << "\nStep 2: Building dstogov/ir Graph\n";
        std::cout << "==================================\n";

        IRBridge bridge;
        IRFunction* fn = bridge.build(values, func.paramTypes);
        
        // bridge.dump(fn);  // disabled for benchmark mode

        // 保存每个函数的 IR
        ir_save(bridge.getCtx(), 0, irFile);
        fflush(irFile);
        std::cout << "Appended IR to: " << irPath << "\n";

        // 輸出 C code: run minimal passes then emit C
        {
            ir_ctx* ctx = bridge.getCtx();
            printf("  pass: def_use_lists\n"); fflush(stdout);
            ir_build_def_use_lists(ctx);
            printf("  pass: cfg\n"); fflush(stdout);
            ir_build_cfg(ctx);
            printf("  pass: dominators\n"); fflush(stdout);
            ir_build_dominators_tree(ctx);
            printf("  pass: loops\n"); fflush(stdout);
            ir_find_loops(ctx);
            printf("  pass: gcm\n"); fflush(stdout);
            ir_gcm(ctx);
            printf("  pass: schedule\n"); fflush(stdout);
            ir_schedule(ctx);
            printf("  pass: vregs\n"); fflush(stdout);
            ir_assign_virtual_registers(ctx);
            printf("  pass: live_ranges\n"); fflush(stdout);
            ir_compute_live_ranges(ctx);
            printf("  pass: coalesce\n"); fflush(stdout);
            ir_coalesce(ctx);
            std::string cname = sanitize_cname(func.name);
            char tmpPath[256];
            snprintf(tmpPath, sizeof(tmpPath), "/tmp/wasm2sea_%zu.c", i);

            FILE* appendFile = fopen(cPath.c_str(), "a");
            ir_emit_c(ctx, cname.c_str(), appendFile);
            fprintf(appendFile, "\n");
            fclose(appendFile);
        }

        // 清理
        delete fn;
    }
    fclose(irFile);
    std::cout << "Saved IR to: " << irPath << "\n";

    // 掃描 out.c 找出實際用到的 local_N，動態生成 header
    {
        std::ifstream scan_f(cPath);
        std::string scan_content((std::istreambuf_iterator<char>(scan_f)),
                                  std::istreambuf_iterator<char>());
        scan_f.close();

        std::set<int> used_locals;
        std::regex local_re("local_(\\d+)");
        auto begin = std::sregex_iterator(scan_content.begin(), scan_content.end(), local_re);
        auto end_it = std::sregex_iterator();
        for (auto it = begin; it != end_it; ++it)
            used_locals.insert(std::stoi((*it)[1].str()));

        std::string header = "#include <stdint.h>\n#include <stdbool.h>\n\n";
        for (int idx : used_locals)
            header += "static int32_t local_" + std::to_string(idx) + ";\n";
        for (int _gi = 0; _gi < g_wasm_global_count; _gi++)
            header += "static int32_t wasm_global_" + std::to_string(_gi) + ";\n";
        header += "\n";

        std::ofstream out_f(cPath);
        out_f << header << scan_content;
        out_f.close();
    }

    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "=== Compilation Complete ===\n";
    std::cout << "Successfully processed " << functions.size() << " function(s)\n";
    std::cout << std::string(70, '=') << "\n";

    return 0;
}
