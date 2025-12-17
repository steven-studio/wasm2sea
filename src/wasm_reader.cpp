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
class WasmToInstrSeqConverter : public Visitor<WasmToInstrSeqConverter> {
public:
    InstrSeq instructions;
    void visitExpression(Expression* curr) {
        // 手动控制遍历顺序
        
        if (auto* localGet = curr->dynCast<LocalGet>()) {
            instructions.push_back({WasmOp::LocalGet, (int)localGet->index});
        }
        else if (auto* cnst = curr->dynCast<Const>()) {
            if (cnst->type == Type::i32) {
                instructions.push_back({WasmOp::I32Const, cnst->value.geti32()});
            }
        }
        else if (auto* bin = curr->dynCast<Binary>()) {
            // 后序遍历：先左后右再操作符
            visitExpression(bin->left);
            visitExpression(bin->right);
            
            switch (bin->op) {
            // 算术运算
            case AddInt32: instructions.push_back({WasmOp::I32Add, 0}); break;
            case SubInt32: instructions.push_back({WasmOp::I32Sub, 0}); break;
            case MulInt32: instructions.push_back({WasmOp::I32Mul, 0}); break;
            case DivSInt32: instructions.push_back({WasmOp::I32DivS, 0}); break;
            case DivUInt32: instructions.push_back({WasmOp::I32DivU, 0}); break;
            case RemSInt32: instructions.push_back({WasmOp::I32RemS, 0}); break;
            case RemUInt32: instructions.push_back({WasmOp::I32RemU, 0}); break;
            
            // 比较运算
            case EqInt32: instructions.push_back({WasmOp::I32Eq, 0}); break;
            case NeInt32: instructions.push_back({WasmOp::I32Ne, 0}); break;
            case LtSInt32: instructions.push_back({WasmOp::I32LtS, 0}); break;
            case LtUInt32: instructions.push_back({WasmOp::I32LtU, 0}); break;
            case GtSInt32: instructions.push_back({WasmOp::I32GtS, 0}); break;
            case GtUInt32: instructions.push_back({WasmOp::I32GtU, 0}); break;
            case LeSInt32: instructions.push_back({WasmOp::I32LeS, 0}); break;
            case LeUInt32: instructions.push_back({WasmOp::I32LeU, 0}); break;
            case GeSInt32: instructions.push_back({WasmOp::I32GeS, 0}); break;
            case GeUInt32: instructions.push_back({WasmOp::I32GeU, 0}); break;
            
            // 位运算
            case AndInt32: instructions.push_back({WasmOp::I32And, 0}); break;
            case OrInt32: instructions.push_back({WasmOp::I32Or, 0}); break;
            case XorInt32: instructions.push_back({WasmOp::I32Xor, 0}); break;
            case ShlInt32: instructions.push_back({WasmOp::I32Shl, 0}); break;
            case ShrSInt32: instructions.push_back({WasmOp::I32ShrS, 0}); break;
            case ShrUInt32: instructions.push_back({WasmOp::I32ShrU, 0}); break;

            default:
                fprintf(stderr, "Warning: Unsupported binary op: %d\n", bin->op);
                break;
            }
        }
        else if (auto* unary = curr->dynCast<Unary>()) {
            visitExpression(unary->value);
            switch (unary->op) {
            case EqZInt32: instructions.push_back({WasmOp::I32Eqz, 0}); break;
            case ClzInt32: instructions.push_back({WasmOp::I32Clz, 0}); break;
            case CtzInt32: instructions.push_back({WasmOp::I32Ctz, 0}); break;
            case PopcntInt32: instructions.push_back({WasmOp::I32Popcnt, 0}); break;
            default:
                fprintf(stderr, "Warning: Unsupported unary op: %d\n", unary->op);
                break;
            }
        }
        else if (auto* ifExpr = curr->dynCast<If>()) {
            // 先访问条件
            visitExpression(ifExpr->condition);
            
            // If 指令
            instructions.push_back({WasmOp::If, 0});
            
            // Then 分支
            visitExpression(ifExpr->ifTrue);
            
            // Else 分支
            if (ifExpr->ifFalse) {
                instructions.push_back({WasmOp::Else, 0});
                visitExpression(ifExpr->ifFalse);
            }
            
            // End
            instructions.push_back({WasmOp::End, 0});
        }
        else if (auto* block = curr->dynCast<Block>()) {
            for (auto* expr : block->list) {
                visitExpression(expr);
            }
        }
        else if (auto* ret = curr->dynCast<Return>()) {
            if (ret->value) {
                visitExpression(ret->value);
            }
            instructions.push_back({WasmOp::Return, 0});
        }
        else if (auto* localSet = curr->dynCast<LocalSet>()) {
            visitExpression(localSet->value);
            if (localSet->isTee()) {
                instructions.push_back({WasmOp::LocalTee, (int)localSet->index});
            } else {
                instructions.push_back({WasmOp::LocalSet, (int)localSet->index});
            }
        }
        else if (auto* select = curr->dynCast<Select>()) {
            visitExpression(select->ifFalse);
            visitExpression(select->ifTrue);
            visitExpression(select->condition);
            instructions.push_back({WasmOp::Select, 0});
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
    // converter.numParams = numParams;  // ← 删除这行，Visitor 不需要
    converter.visitExpression(func->body);  // ← 改用 visitExpression
    
    // // 添加调试输出：
    // printf("Debug: Generated instructions:\n");
    // for (size_t i = 0; i < converter.instructions.size(); i++) {
    //     printf("  [%zu] op=%d, operand=%d\n", 
    //         i, (int)converter.instructions[i].op, 
    //         converter.instructions[i].operand);
    // }

    // 构建返回结果
    InstrSeq result;
    result.push_back({WasmOp::FuncInfo, (int)numParams});  // 第一条指令存储参数数量
    result.insert(result.end(), 
                  converter.instructions.begin(), 
                  converter.instructions.end());
    
    printf("Converted %zu instructions\n", result.size());
    
    return result;
}