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
    Module* modulePtr = nullptr;
    std::vector<Name> label_stack;  // 添加标签栈
    
    // 查找标签深度
    int getLabelDepth(Name label) {
        // 从栈顶往下找（最内层是 0）
        for (int i = (int)label_stack.size() - 1; i >= 0; i--) {
            if (label_stack[i] == label) {
                return (int)label_stack.size() - 1 - i;
            }
        }
        return -1;  // 未找到
    }

    // 每個 node type 變成獨立 private method
    void visitLocalGet(LocalGet* n) {
        instructions.push_back({WasmOp::LocalGet, (int)n->index});
    }

    int getGlobalIndex(Name name) {
        auto& globals = modulePtr->globals;
        for (int i = 0; i < (int)globals.size(); i++) {
            if (globals[i]->name == name) return i;
        }
        return -1;
    }

    void visitGlobalGet(GlobalGet* n) {
        instructions.push_back({WasmOp::GlobalGet, getGlobalIndex(n->name)});
    }

    void visitGlobalSet(GlobalSet* n) {
        visitExpression(n->value);
        instructions.push_back({WasmOp::GlobalSet, getGlobalIndex(n->name)});
    }

    void visitLoad(Load* n) {
        visitExpression(n->ptr);
        Instr instr;
        instr.op = (n->type == Type::f64) ? WasmOp::F64Load : WasmOp::I32Load;
        instr.operand = (int)n->offset;
        instructions.push_back(instr);
    }

    void visitConst(Const* n) {
        Instr instr;
        if (n->type == Type::i32) {
            instr.op = WasmOp::I32Const;
            instr.operand = n->value.geti32();
        } else if (n->type == Type::i64) {
            instr.op = WasmOp::I64Const;
            instr.i64operand = n->value.geti64();
        } else if (n->type == Type::f64) {
            instr.op = WasmOp::F64Const;
            instr.foperand = n->value.getf64();
        } else {
            fprintf(stderr, "Warning: Unsupported const type\n");
            return;
        }
        instructions.push_back(instr);
    }

    void visitLocalSet(LocalSet* n) {
        visitExpression(n->value);
        if (n->isTee()) {
            instructions.push_back({WasmOp::LocalTee, (int)n->index});
        } else {
            instructions.push_back({WasmOp::LocalSet, (int)n->index});
        }
    }

    void visitStore(Store* n) {
        visitExpression(n->ptr);
        visitExpression(n->value);
        Instr instr;
        instr.op = (n->valueType == Type::f64) ? WasmOp::F64Store : WasmOp::I32Store;
        instr.operand = (int)n->offset;
        instructions.push_back(instr);
    }

    void visitBinary(Binary* n) {
        visitExpression(n->left);
        visitExpression(n->right);
        instructions.push_back({binaryOpToWasmOp(n->op), 0});
    }

    void visitUnary(Unary* n) {
        visitExpression(n->value);
        WasmOp op = unaryOpToWasmOp(n->op);
        if (op == WasmOp::Unsupported)
            fprintf(stderr, "Warning: Unsupported unary op: %d\n", n->op);
        instructions.push_back({op, 0});
    }

    void visitSelect(Select* n) {
        visitExpression(n->ifFalse);
        visitExpression(n->ifTrue);
        visitExpression(n->condition);
        instructions.push_back({WasmOp::Select, 0});
    }

    void visitDrop(Drop* n) {
        visitExpression(n->value);
        instructions.push_back({WasmOp::Drop, 0});
    }

    int getFunctionIndex(Name name) {
        for (int i = 0; i < (int)modulePtr->functions.size(); i++) {
            if (modulePtr->functions[i]->name == name) return i;
        }
        return -1;
    }

    void visitCall(Call* n) {
        for (auto* operand : n->operands) {
            visitExpression(operand);
        }
        Instr instr;
        instr.op = WasmOp::Call;
        instr.operand = getFunctionIndex(n->target);
        instr.foperand = (double)n->operands.size();
        instructions.push_back(instr);
    }

    void visitIf(If* n) {
        visitExpression(n->condition);
        instructions.push_back({WasmOp::If, 0});
        visitExpression(n->ifTrue);
        if (n->ifFalse) {
            instructions.push_back({WasmOp::Else, 0});
            visitExpression(n->ifFalse);
        }
        instructions.push_back({WasmOp::End, 0});
    }

    void visitLoop(Loop* n) {
        label_stack.push_back(n->name);
        instructions.push_back({WasmOp::Loop, 0});
        visitExpression(n->body);
        instructions.push_back({WasmOp::End, 0});
        label_stack.pop_back();
    }

    void visitBreak(Break* n) {
        int depth = getLabelDepth(n->name);
        if (depth < 0) {
            fprintf(stderr, "Error: Unknown label in br: %s\n",
                    std::string(n->name.str).c_str());
            return;
        }
        if (n->condition) {
            visitExpression(n->condition);
            instructions.push_back({WasmOp::Br_if, depth});
        } else {
            instructions.push_back({WasmOp::Br, depth});
        }
    }

    void visitSwitch(Switch* n) {
        visitExpression(n->condition);
        int depth = getLabelDepth(n->default_);
        if (depth < 0) depth = 0;
        instructions.push_back({WasmOp::BrTable, depth});
    }

    void visitBlock(Block* n) {
        if (n->name.is()) label_stack.push_back(n->name);
        for (auto* expr : n->list) {
            visitExpression(expr);
        }
        if (n->name.is()) label_stack.pop_back();
    }

    void visitReturn(Return* n) {
        if (n->value) visitExpression(n->value);
        instructions.push_back({WasmOp::Return, 0});
    }

    void visitUnreachable(Unreachable* n) {
        instructions.push_back({WasmOp::Unreachable, 0});
    }

    void visitMemorySize(MemorySize* n) {
        instructions.push_back({WasmOp::MemorySize, 0});
    }

    void visitMemoryCopy(MemoryCopy* n) {
        visitExpression(n->dest);
        visitExpression(n->source);
        visitExpression(n->size);
        instructions.push_back({WasmOp::MemoryCopy, 0});
    }

    void visitMemoryFill(MemoryFill* n) {
        visitExpression(n->dest);
        visitExpression(n->value);
        visitExpression(n->size);
        instructions.push_back({WasmOp::MemoryFill, 0});
    }

    void visitExpression(Expression* curr) {
        if (!curr) return;
        visit(curr);  // Binaryen 自動 dispatch 到對應的 visitXxx
    }

    static WasmOp binaryOpToWasmOp(BinaryOp op) {
        switch (op) {
            // i32 算術
            case AddInt32:   return WasmOp::I32Add;
            case SubInt32:   return WasmOp::I32Sub;
            case MulInt32:   return WasmOp::I32Mul;
            case DivSInt32:  return WasmOp::I32DivS;
            case DivUInt32:  return WasmOp::I32DivU;
            case RemSInt32:  return WasmOp::I32RemS;
            case RemUInt32:  return WasmOp::I32RemU;

            // i32 比較
            case EqInt32:    return WasmOp::I32Eq;
            case NeInt32:    return WasmOp::I32Ne;
            case LtSInt32:   return WasmOp::I32LtS;
            case LtUInt32:   return WasmOp::I32LtU;
            case GtSInt32:   return WasmOp::I32GtS;
            case GtUInt32:   return WasmOp::I32GtU;
            case LeSInt32:   return WasmOp::I32LeS;
            case LeUInt32:   return WasmOp::I32LeU;
            case GeSInt32:   return WasmOp::I32GeS;
            case GeUInt32:   return WasmOp::I32GeU;

            // i32 位元
            case AndInt32:   return WasmOp::I32And;
            case OrInt32:    return WasmOp::I32Or;
            case XorInt32:   return WasmOp::I32Xor;
            case ShlInt32:   return WasmOp::I32Shl;
            case ShrSInt32:  return WasmOp::I32ShrS;
            case ShrUInt32:  return WasmOp::I32ShrU;

            // i64 算術
            case AddInt64:   return WasmOp::I64Add;
            case SubInt64:   return WasmOp::I64Sub;
            case MulInt64:   return WasmOp::I64Mul;
            case DivSInt64:  return WasmOp::I64DivS;
            case DivUInt64:  return WasmOp::I64DivU;
            case RemSInt64:  return WasmOp::I64RemS;
            case RemUInt64:  return WasmOp::I64RemU;

            // i64 位元
            case AndInt64:   return WasmOp::I64And;
            case OrInt64:    return WasmOp::I64Or;
            case XorInt64:   return WasmOp::I64Xor;
            case ShlInt64:   return WasmOp::I64Shl;
            case ShrSInt64:  return WasmOp::I64ShrS;
            case ShrUInt64:  return WasmOp::I64ShrU;

            // i64 比較
            case EqInt64:    return WasmOp::I64Eq;
            case NeInt64:    return WasmOp::I64Ne;
            case LtSInt64:   return WasmOp::I64LtS;
            case LtUInt64:   return WasmOp::I64LtU;
            case GtSInt64:   return WasmOp::I64GtS;
            case GtUInt64:   return WasmOp::I64GtU;
            case LeSInt64:   return WasmOp::I64LeS;
            case LeUInt64:   return WasmOp::I64LeU;
            case GeSInt64:   return WasmOp::I64GeS;
            case GeUInt64:   return WasmOp::I64GeU;

            // f64 算術
            case AddFloat64: return WasmOp::F64Add;
            case SubFloat64: return WasmOp::F64Sub;
            case MulFloat64: return WasmOp::F64Mul;
            case DivFloat64: return WasmOp::F64Div;
            case EqFloat64:  return WasmOp::F64Eq;
            case NeFloat64:  return WasmOp::F64Ne;

            // f64 比較
            case LtFloat64:  return WasmOp::F64Lt;
            case GtFloat64:  return WasmOp::F64Gt;
            case LeFloat64:  return WasmOp::F64Le;
            case GeFloat64:  return WasmOp::F64Ge;
            case MinFloat64: return WasmOp::F64Min;
            case MaxFloat64: return WasmOp::F64Max;

            default: return WasmOp::Unsupported;
        }
    }

    static WasmOp unaryOpToWasmOp(UnaryOp op) {
        switch (op) {
            // i32
            case EqZInt32:   return WasmOp::I32Eqz;
            case ClzInt32:   return WasmOp::I32Clz;
            case CtzInt32:   return WasmOp::I32Ctz;
            case PopcntInt32: return WasmOp::I32Popcnt;

            // i64
            case EqZInt64:    return WasmOp::I64Eqz;
            case ClzInt64:    return WasmOp::I64Clz;
            case CtzInt64:    return WasmOp::I64Ctz;
            case PopcntInt64: return WasmOp::I64Popcnt;

            // i64 <-> i32 轉換
            case WrapInt64:              return WasmOp::I32WrapI64;
            case ExtendSInt32:           return WasmOp::I64ExtendI32S;
            case ExtendUInt32:           return WasmOp::I64ExtendI32U;

            // i64 <-> f64 轉換
            case ConvertSInt64ToFloat64: return WasmOp::F64ConvertI64S;
            case ConvertUInt64ToFloat64: return WasmOp::F64ConvertI64U;
            case TruncSFloat64ToInt64:   return WasmOp::I64TruncF64S;
            case TruncUFloat64ToInt64:   return WasmOp::I64TruncF64U;

            // f64 算術
            case AbsFloat64:  return WasmOp::F64Abs;
            case NegFloat64:  return WasmOp::F64Neg;
            case SqrtFloat64: return WasmOp::F64Sqrt;

            // 型別轉換
            case ConvertSInt32ToFloat64: return WasmOp::F64ConvertI32S;
            case ConvertUInt32ToFloat64: return WasmOp::F64ConvertI32U;
            case TruncSFloat64ToInt32:   return WasmOp::I32TruncF64S;
            case TruncUFloat64ToInt32:   return WasmOp::I32TruncF64U;

            default: return WasmOp::Unsupported;
        }
    }
};

// 修改返回类型：从 InstrSeq 改为 vector<FunctionResult>
std::vector<FunctionResult> readWasmFile(const std::string& filename) {
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
    
    printf("Module has %zu functions\n", module.functions.size());
    
    // ✅ 步骤 1: 先构建函数索引到导出名的映射
    std::map<size_t, std::string> functionExports;
    
    printf("\n=== Debug: Building function export map ===\n");
    printf("Total exports: %zu\n", module.exports.size());

    size_t exportIndex = 0;
    for (auto& exp : module.exports) {
        printf("Export[%zu]: name='%s', kind=%d\n", 
            exportIndex, exp->name.str.data(), (int)exp->kind);

        if (exp->kind == ExternalKind::Function) {
            if (exportIndex < module.functions.size()) {
                functionExports[exportIndex] = exp->name.str;
                printf("  -> ✓ Mapped: functionExports[%zu] = '%s'\n", 
                    exportIndex, exp->name.str.data());
            } else {
                printf("  -> ✗ Warning: exportIndex >= functions.size()\n");
            }
            exportIndex++;
        }
    }

    printf("=== Total mapped: %zu functions ===\n\n", functionExports.size());

    // 存储所有函数的转换结果
    std::vector<FunctionResult> results;

    for (size_t i = 0; i < module.functions.size(); i++) {
        if (!module.functions[i]) {
            fprintf(stderr, "Error: Null function at index %zu\n", i);
            return {};
        }

        Function* func = module.functions[i].get();

        if (!func) {
            fprintf(stderr, "Error: Function pointer is null\n");
            return {};
        }

        // 获取函数名（这部分你也缺少了）
        std::string funcName;
        // 优先：使用导出名
        if (functionExports.find(i) != functionExports.end()) {
            funcName = functionExports[i];
        }
        // 次选：使用内部名
        else if (func->name.is()) {
            funcName = std::string(func->name.str);
        }
        // 后备：使用索引
        else {
            funcName = "func_" + std::to_string(i);
        }

        printf("Processing function [%zu]: %s\n", i, funcName.c_str());

        // 获取参数数量
        size_t numParams = func->getNumParams();
        printf("Function has %zu parameters\n", numParams);
        
        // 使用 walker 轉換
        WasmToInstrSeqConverter converter;
        converter.modulePtr = &module;
        // converter.numParams = numParams;  // ← 删除这行，Visitor 不需要
        converter.visitExpression(func->body);  // ← 改用 visitExpression

        // 构建返回结果
        InstrSeq instrSeq;
        instrSeq.push_back({WasmOp::FuncInfo, (int)numParams});  // 第一条指令存储参数数量
        instrSeq.insert(instrSeq.end(), 
                    converter.instructions.begin(), 
                    converter.instructions.end());

        // ✅ 关键：创建 FunctionResult 并添加到 results
        FunctionResult funcResult;
        funcResult.name = funcName;           // ← 保存函数名
        funcResult.numParams = numParams;     // ← 保存参数数量
        funcResult.instructions = instrSeq;   // ← 保存指令序列
        fprintf(stderr, "DEBUG: filling paramTypes, numParams=%zu\n", func->getNumParams());
        for (size_t j = 0; j < func->getNumParams(); j++) {
            wasm::Type t = func->getLocalType(j);
            fprintf(stderr, "DEBUG: param[%zu] type=%s\n", j, t.toString().c_str());
            if (t == wasm::Type::i64)      funcResult.paramTypes.push_back(ParamType::I64);
            else if (t == wasm::Type::f64) funcResult.paramTypes.push_back(ParamType::F64);
            else                            funcResult.paramTypes.push_back(ParamType::I32);
        }
        fprintf(stderr, "DEBUG: paramTypes.size()=%zu\n", funcResult.paramTypes.size());

        results.push_back(funcResult);        // ← 添加到 results！
        
        printf("  Converted %zu instructions\n", instrSeq.size());
    }
    
    printf("Successfully converted %zu functions\n", results.size());
    
    return results;
}
