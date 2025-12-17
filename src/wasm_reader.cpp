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
    size_t numParams = 0;  // 新增：存储参数数量

    void visitLocalGet(LocalGet* curr) {
        instructions.push_back({WasmOp::LocalGet, (int)curr->index});
    }
    
    void visitLocalSet(LocalSet* curr) {
        // 先處理要設定的值
        visit(curr->value);
        // TODO: 如果需要 LocalSet 指令
        // 使用 isTee() 判断是 local.tee 还是 local.set
        if (curr->isTee()) {
            instructions.push_back({WasmOp::LocalTee, (int)curr->index});
        } else {
            instructions.push_back({WasmOp::LocalSet, (int)curr->index});
        }
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

        case AndInt32:
            instructions.push_back({WasmOp::I32And, 0});
            break;
        case OrInt32:
            instructions.push_back({WasmOp::I32Or, 0});
            break;
        case XorInt32:
            instructions.push_back({WasmOp::I32Xor, 0});
            break;
        case ShlInt32:
            instructions.push_back({WasmOp::I32Shl, 0});
            break;
        case ShrSInt32:
            instructions.push_back({WasmOp::I32ShrS, 0});
            break;
        case ShrUInt32:
            instructions.push_back({WasmOp::I32ShrU, 0});
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
        case ClzInt32:
            instructions.push_back({WasmOp::I32Clz, 0});
            break;
        case CtzInt32:
            instructions.push_back({WasmOp::I32Ctz, 0});
            break;
        case PopcntInt32:
            instructions.push_back({WasmOp::I32Popcnt, 0});
            break;

        default:
            fprintf(stderr, "Warning: Unsupported unary op: %d\n", curr->op);
            break;
        }
    }

    void visitSelect(Select* curr) {
        // Binaryen 的 Select 有 3 个操作数
        // condition, ifTrue, ifFalse
        
        // 按照栈的顺序访问
        visit(curr->ifFalse);    // 先访问 false
        visit(curr->ifTrue);     // 再访问 true
        visit(curr->condition);  // 最后访问 condition
        
        instructions.push_back({WasmOp::Select, 0});
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

    // 获取参数数量
    size_t numParams = func->getNumParams();
    printf("Function has %zu parameters\n", numParams);
    
    // 使用 walker 轉換
    WasmToInstrSeqConverter converter;
    converter.numParams = numParams;  // 传递参数数量
    converter.walk(func->body);
    
    // 构建返回结果
    InstrSeq result;
    result.push_back({WasmOp::FuncInfo, (int)numParams});  // 第一条指令存储参数数量
    result.insert(result.end(), 
                  converter.instructions.begin(), 
                  converter.instructions.end());
    
    printf("Converted %zu instructions\n", result.size());
    
    return result;
}