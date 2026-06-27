#pragma once

#include "wasm_reader.hpp"
#include "wasm.h"
#include "wasm-binary.h"
#include "wasm-builder.h"
#include "ir/module-utils.h"

using namespace wasm;

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
        int cidx = getFunctionIndex(n->target);
//        fprintf(stderr, "[CALL] target name=%s, index=%d\n", std::string(n->target.str).c_str(), cidx);
        instr.operand = cidx;
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
        instructions.push_back({WasmOp::End, 2});
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
        if (n->name.is()) {
            label_stack.push_back(n->name);
            instructions.push_back({WasmOp::Block, (int)label_stack.size()});  // ← 加這行
        }
        for (auto* expr : n->list) {
            visitExpression(expr);
        }
        if (n->name.is()) {
            label_stack.pop_back();
            instructions.push_back({WasmOp::End, 1});  // ← 加對應的 End
        }
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
