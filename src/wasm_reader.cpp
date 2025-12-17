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
        case DivSInt32:  // ← 新增：有號除法
            instructions.push_back({WasmOp::I32DivS, 0});
            break;
        case DivUInt32:  // ← 新增：無號除法
            instructions.push_back({WasmOp::I32DivU, 0});
            break;
        case RemSInt32:  // ← 新增：有號取餘
            instructions.push_back({WasmOp::I32RemS, 0});
            break;
        case RemUInt32:  // ← 新增：無號取餘
            instructions.push_back({WasmOp::I32RemU, 0});
            break;

        // 比較運算（新增）
        case EqInt32:
            instructions.push_back({WasmOp::I32Eq, 0});
            break;
        case NeInt32:
            instructions.push_back({WasmOp::I32Ne, 0});
            break;
        case LtSInt32:
            instructions.push_back({WasmOp::I32LtS, 0});
            break;
        case LtUInt32:
            instructions.push_back({WasmOp::I32LtU, 0});
            break;
        case GtSInt32:
            instructions.push_back({WasmOp::I32GtS, 0});
            break;
        case GtUInt32:
            instructions.push_back({WasmOp::I32GtU, 0});
            break;
        case LeSInt32:
            instructions.push_back({WasmOp::I32LeS, 0});
            break;
        case LeUInt32:
            instructions.push_back({WasmOp::I32LeU, 0});
            break;
        case GeSInt32:
            instructions.push_back({WasmOp::I32GeS, 0});
            break;
        case GeUInt32:
            instructions.push_back({WasmOp::I32GeU, 0});
            break;

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
    
    // 新增：處理 Unary 運算（eqz 是一元運算）
    void visitUnary(Unary* curr) {
        visit(curr->value);
        
        switch (curr->op) {
        case EqZInt32:
            instructions.push_back({WasmOp::I32Eqz, 0});
            break;
        default:
            fprintf(stderr, "Warning: Unsupported unary op: %d\n", curr->op);
            break;
        }
    }

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