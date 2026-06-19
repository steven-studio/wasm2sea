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

    void visitExpression(Expression* curr) {
        // 手动控制遍历顺序
        if (!curr) return;
        if (auto* localGet = curr->dynCast<LocalGet>()) {
            instructions.push_back({WasmOp::LocalGet, (int)localGet->index});
        }
        else if (auto* cnst = curr->dynCast<Const>()) {
            if (cnst->type == Type::i32) {
                instructions.push_back({WasmOp::I32Const, cnst->value.geti32()});
            } else if (cnst->type == Type::f64) {
                Instr instr;
                instr.op = WasmOp::F64Const;
                instr.foperand = cnst->value.getf64();
                instructions.push_back(instr);
            }
        }
        else if (auto* load = curr->dynCast<Load>()) {
            visitExpression(load->ptr);
            Instr instr;
            if (load->type == Type::f64) {
                instr.op = WasmOp::F64Load;
            } else {
                instr.op = WasmOp::I32Load;
            }
            instr.operand = (int)load->offset;
            instructions.push_back(instr);
        }
        else if (auto* store = curr->dynCast<Store>()) {
            visitExpression(store->ptr);
            visitExpression(store->value);
            Instr instr;
            if (store->valueType == Type::f64) {
                instr.op = WasmOp::F64Store;
            } else {
                instr.op = WasmOp::I32Store;
            }
            instr.operand = (int)store->offset;
            instructions.push_back(instr);
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
            case AddFloat64: instructions.push_back({WasmOp::F64Add, 0}); break;
            case SubFloat64: instructions.push_back({WasmOp::F64Sub, 0}); break;
            case MulFloat64: instructions.push_back({WasmOp::F64Mul, 0}); break;
            case DivFloat64: instructions.push_back({WasmOp::F64Div, 0}); break;
            case EqFloat64:  instructions.push_back({WasmOp::F64Eq,  0}); break;
            case NeFloat64:  instructions.push_back({WasmOp::F64Ne,  0}); break;
            case LtFloat64:  instructions.push_back({WasmOp::F64Lt,  0}); break;
            case GtFloat64:  instructions.push_back({WasmOp::F64Gt,  0}); break;
            case LeFloat64:  instructions.push_back({WasmOp::F64Le,  0}); break;
            case GeFloat64:  instructions.push_back({WasmOp::F64Ge,  0}); break;
            case MinFloat64: instructions.push_back({WasmOp::F64Min, 0}); break;
            case MaxFloat64: instructions.push_back({WasmOp::F64Max, 0}); break;
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
            case AbsFloat64:  instructions.push_back({WasmOp::F64Abs,  0}); break;
            case NegFloat64:  instructions.push_back({WasmOp::F64Neg,  0}); break;
            case SqrtFloat64: instructions.push_back({WasmOp::F64Sqrt, 0}); break;
            case ConvertSInt32ToFloat64: instructions.push_back({WasmOp::F64ConvertI32S, 0}); break;
            case ConvertUInt32ToFloat64: instructions.push_back({WasmOp::F64ConvertI32U, 0}); break;
            case TruncSFloat64ToInt32:   instructions.push_back({WasmOp::I32TruncF64S, 0}); break;
            case TruncUFloat64ToInt32:   instructions.push_back({WasmOp::I32TruncF64U, 0}); break;
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
        else if (auto* loop = curr->dynCast<Loop>()) {
            label_stack.push_back(loop->name);  // 压入标签
            instructions.push_back({WasmOp::Loop, 0});
            visitExpression(loop->body);
            instructions.push_back({WasmOp::End, 0});
            label_stack.pop_back();  // 弹出标签
        }
        else if (auto* br = curr->dynCast<Break>()) {
            int depth = getLabelDepth(br->name);
            if (depth < 0) {
                fprintf(stderr, "Error: Unknown label in br: %s\n", 
                        std::string(br->name.str).c_str());
                return;
            }
            
            if (br->condition) {
                // br_if: 先计算条件
                visitExpression(br->condition);
                instructions.push_back({WasmOp::Br_if, depth});  // 使用深度
            } else {
                // br: 无条件跳转
                instructions.push_back({WasmOp::Br, depth});  // 使用深度
            }
        }
        else if (auto* block = curr->dynCast<Block>()) {
            if (block->name.is()) label_stack.push_back(block->name);
            for (auto* expr : block->list) {
                visitExpression(expr);
            }
            if (block->name.is()) label_stack.pop_back();
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
        else if (auto* globalGet = curr->dynCast<GlobalGet>()) {
            // 找 global 的 index
            auto& globals = modulePtr->globals;
            int idx = 0;
            for (auto& g : globals) {
                if (g->name == globalGet->name) break;
                idx++;
            }
            instructions.push_back({WasmOp::GlobalGet, idx});
        }
        else if (auto* globalSet = curr->dynCast<GlobalSet>()) {
            visitExpression(globalSet->value);
            auto& globals = modulePtr->globals;
            int idx = 0;
            for (auto& g : globals) {
                if (g->name == globalSet->name) break;
                idx++;
            }
            instructions.push_back({WasmOp::GlobalSet, idx});
        }
        else if (auto* select = curr->dynCast<Select>()) {
            visitExpression(select->ifFalse);
            visitExpression(select->ifTrue);
            visitExpression(select->condition);
            instructions.push_back({WasmOp::Select, 0});
        }
        else if (curr->is<wasm::Drop>()) {
            visitExpression(curr->cast<wasm::Drop>()->value);
            instructions.push_back({WasmOp::Drop, 0});
        }
        else if (auto* call = curr->dynCast<wasm::Call>()) {
            // 先 visit 所有參數（push 到 stack）
            for (auto* operand : call->operands) {
                visitExpression(operand);
            }
            // 找 callee 的 function index
            int calleeIdx = -1;
            for (int i = 0; i < (int)modulePtr->functions.size(); i++) {
                if (modulePtr->functions[i]->name == call->target) {
                    calleeIdx = i;
                    break;
                }
            }
            Instr instr;
            instr.op = WasmOp::Call;
            instr.operand = calleeIdx;
            instr.foperand = (double)call->operands.size();
            instructions.push_back(instr);
        }
        else if (auto* sw = curr->dynCast<Switch>()) {
            // 先計算 index
            visitExpression(sw->condition);
            // 用 default target 的 depth
            int depth = getLabelDepth(sw->default_);
            if (depth < 0) depth = 0;
            instructions.push_back({WasmOp::BrTable, depth});
        }
        else if (curr->is<wasm::Unreachable>()) {
            instructions.push_back({WasmOp::Unreachable, 0});
        }
        else if (curr->is<wasm::MemorySize>()) {
            instructions.push_back({WasmOp::MemorySize, 0});
        }
        else if (curr->is<wasm::MemoryCopy>()) {
            auto* mc = curr->cast<wasm::MemoryCopy>();
            visitExpression(mc->dest);
            visitExpression(mc->source);
            visitExpression(mc->size);
            instructions.push_back({WasmOp::MemoryCopy, 0});
        }
        else if (curr->is<wasm::MemoryFill>()) {
            auto* mf = curr->cast<wasm::MemoryFill>();
            visitExpression(mf->dest);
            visitExpression(mf->value);
            visitExpression(mf->size);
            instructions.push_back({WasmOp::MemoryFill, 0});
        }
        else {
            // Unsupported instruction, skip
            std::cerr << "[WARN] Unsupported expression type: " 
                    << wasm::getExpressionName(curr) << "\n";
            instructions.push_back({WasmOp::Unsupported, 0});
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
        
        results.push_back(funcResult);        // ← 添加到 results！
        
        printf("  Converted %zu instructions\n", instrSeq.size());
    }
    
    printf("Successfully converted %zu functions\n", results.size());
    
    return results;
}
