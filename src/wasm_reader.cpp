#include "wasm_reader.hpp"
#include <fstream>
#include <cstdio>
#include <cstdint>
#include <vector>

// Binaryen headers
#include "wasm.h"
#include "wasm-binary.h"
#include "wasm-builder.h"
#include "ir/module-utils.h"

using namespace wasm;

// 輔助函數：遍歷 Binaryen Expression 並轉換為 InstrSeq
class WasmToInstrSeqConverter : public ExpressionStackWalker<WasmToInstrSeqConverter> {
public:
    InstrSeq instructions;
    
    void visitLocalGet(LocalGet* curr) {
        instructions.push_back({WasmOp::LocalGet, (int)curr->index});
    }
    
    void visitLocalSet(LocalSet* curr) {
        // 先處理要設定的值
        visit(curr->value);
        // TODO: 如果需要 LocalSet 指令
    }
    
    void visitBinary(Binary* curr) {
        // 先處理左右運算元
        visit(curr->left);
        visit(curr->right);
        
        // 根據運算類型產生指令
        switch (curr->op) {
        case AddInt32:
            instructions.push_back({WasmOp::I32Add, 0});
            break;
        case SubInt32:
            instructions.push_back({WasmOp::I32Sub, 0});
            break;
        case MulInt32:
            instructions.push_back({WasmOp::I32Mul, 0});
            break;
        // 可以加更多運算
        default:
            fprintf(stderr, "Warning: Unsupported binary op: %d\n", curr->op);
            break;
        }
    }
    
    void visitConst(Const* curr) {
        if (curr->type == Type::i32) {
            instructions.push_back({WasmOp::I32Const, curr->value.geti32()});
        }
        // 可以加其他類型
    }
    
    void visitReturn(Return* curr) {
        if (curr->value) {
            visit(curr->value);
        }
        instructions.push_back({WasmOp::Return, 0});
    }
    
    // 其他指令的處理...
    void visitBlock(Block* curr) {
        for (auto* expr : curr->list) {
            visit(expr);
        }
    }
};

InstrSeq readWasmFile(const std::string& filename) {
    // 讀取檔案
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename.c_str());
        return {};
    }
    
    std::vector<char> buffer((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
    
    if (buffer.empty()) {
        fprintf(stderr, "Error: Empty file\n");
        return {};
    }
    
    printf("Reading WASM file: %s (%zu bytes)\n", filename.c_str(), buffer.size());
    
    // 使用 Binaryen 解析
    Module module;
    
    try {
        // 使用 WasmBinaryReader
        WasmBinaryReader reader(
            module,
            FeatureSet::All,
            buffer
        );
        reader.read();
    } catch (ParseException& e) {
        fprintf(stderr, "Error parsing WASM: %s\n", e.text.c_str());
        return {};
    }
    
    // 轉換第一個函數（假設只有一個）
    if (module.functions.empty()) {
        fprintf(stderr, "Error: No functions in module\n");
        return {};
    }
    
    Function* func = module.functions[0].get();

    if (!func) {
        fprintf(stderr, "Error: Function pointer is null\n");
        return {};
    }

    // 安全的方式：先檢查 name 是否有效
    if (func->name.is()) {
        // string_view 轉 string
        std::string funcName(func->name.str);
        printf("Processing function: %s\n", funcName.c_str());
    } else {
        printf("Processing function: <unnamed>\n");
    }
    
    // 使用 walker 轉換
    WasmToInstrSeqConverter converter;
    converter.walk(func->body);
    
    printf("Converted %zu instructions\n", converter.instructions.size());
    
    return converter.instructions;
}