// src/value_ir_verify.hpp
#pragma once
#include "value_ir.hpp"
#include <cstdio>

struct VerifyResult {
    bool ok = true;
    int errorCount = 0;
};

// 邊界檢查：ref 必須是合法的 SSA id（在 ir.size() 範圍內）。
// forwardOk=false 時，額外要求 ref < selfIdx（標準 SSA dominance：
// def 必須先於 use）；loop-carried Phi 的 back-edge operand 允許
// forward reference，呼叫端傳 forwardOk=true 跳過這個限制。
inline void checkRef(const ValueIR& ir, int selfIdx, int ref,
                     const char* fieldName, bool forwardOk,
                     VerifyResult& result) {
    if (ref == -1) return;   // -1 表示「無值」（例如 void Return），合法
    if (ref < 0 || ref >= (int)ir.size()) {
        fprintf(stderr, "v%d: %s=%d out of bounds [0, %zu)\n",
                selfIdx, fieldName, ref, ir.size());
        result.ok = false;
        result.errorCount++;
        return;
    }
    if (!forwardOk && ref >= selfIdx) {
        fprintf(stderr, "v%d: %s=%d is a forward reference "
                        "(SSA def must precede use)\n",
                selfIdx, fieldName, ref);
        result.ok = false;
        result.errorCount++;
    }
}

inline VerifyResult verifyValueIR(const ValueIR& ir) {
    VerifyResult result;

    for (size_t i = 0; i < ir.size(); ++i) {
        const Value& v = ir[i];
        int idx = (int)i;

        switch (v.op) {
            // ---- 標準二元運算 / 比較 / 位元運算：lhs、rhs都是SSA ref
            //      （emitBinary 建立，見 MAKE_BINARY 清單）----
            case Op::Add: case Op::Sub: case Op::Mul:
            case Op::Div_S: case Op::Div_U: case Op::Rem_S: case Op::Rem_U:
            case Op::Eq: case Op::Ne:
            case Op::Lt_S: case Op::Lt_U: case Op::Gt_S: case Op::Gt_U:
            case Op::Le_S: case Op::Le_U: case Op::Ge_S: case Op::Ge_U:
            case Op::And: case Op::Or: case Op::Xor:
            case Op::Shl: case Op::Shr_S: case Op::Shr_U:
            case Op::Rotl: case Op::Rotr:   // 目前 kDispatch 未接，但保留
            case Op::F64Add: case Op::F64Sub: case Op::F64Mul: case Op::F64Div:
            case Op::F64Min: case Op::F64Max:   // 同上，未接但保留
            case Op::F64Eq: case Op::F64Ne: case Op::F64Lt: case Op::F64Gt:
            case Op::F64Le: case Op::F64Ge:
                checkRef(ir, idx, v.lhs, "lhs", false, result);
                checkRef(ir, idx, v.rhs, "rhs", false, result);
                break;

            // ---- 一元運算：只有lhs（emitUnary 建立，見 MAKE_UNARY 清單，
            //      F64Pow 實際上也在這組，不是binary！）----
            case Op::Eqz: case Op::Clz: case Op::Ctz: case Op::Popcnt:
            case Op::F64Abs: case Op::F64Neg: case Op::F64Sqrt:
            case Op::F64Exp: case Op::F64Log: case Op::F64Sin: case Op::F64Cos:
            case Op::F64Pow:
            case Op::F64ConvertI32S: case Op::F64ConvertI32U:
            case Op::I32TruncF64S: case Op::I32TruncF64U:
            case Op::I32WrapI64:
            case Op::I64ExtendI32S: case Op::I64ExtendI32U:
            case Op::F64ConvertI64S: case Op::F64ConvertI64U:
            case Op::I64TruncF64S: case Op::I64TruncF64U:
                checkRef(ir, idx, v.lhs, "lhs", false, result);
                break;

            // ---- Memory：ptr/val 都是SSA ref ----
            case Op::Load: case Op::F64Load:
                checkRef(ir, idx, v.lhs, "ptr", false, result);
                break;
            case Op::Store: case Op::F64Store:
                checkRef(ir, idx, v.lhs, "ptr", false, result);
                checkRef(ir, idx, v.rhs, "val", false, result);
                break;
            case Op::MemoryCopy: case Op::MemoryFill:
                for (int op : v.operands)
                    checkRef(ir, idx, op, "memory op operand", false, result);
                break;
            case Op::MemorySize:
                break;

            // ---- Select：operands = {cond, true_val, false_val} ----
            case Op::Select:
                for (int op : v.operands)
                    checkRef(ir, idx, op, "select operand", false, result);
                break;

            // ---- Call：lhs是callee_idx（opaque），operands是真正args ----
            case Op::Call:
                for (int arg : v.operands)
                    checkRef(ir, idx, arg, "call arg", false, result);
                break;

            // ---- Phi：用local_index分辨if-merge vs loop-carried ----
            case Op::Phi:
                if (v.local_index == -1) {
                    for (int op : v.operands)
                        checkRef(ir, idx, op, "if-merge phi operand",
                                 false, result);
                } else {
                    if (!v.operands.empty())
                        checkRef(ir, idx, v.operands[0], "phi entry operand",
                                 false, result);
                    for (size_t k = 1; k < v.operands.size(); ++k)
                        checkRef(ir, idx, v.operands[k], "phi backedge operand",
                                 true, result);
                }
                break;

            case Op::If:
                checkRef(ir, idx, v.lhs, "if condition", false, result);
                break;

            case Op::Br_if:
                checkRef(ir, idx, v.lhs, "br_if condition", false, result);
                checkRef(ir, idx, v.rhs, "br_if target", false, result);
                break;

            case Op::Br:
                checkRef(ir, idx, v.lhs, "br target", false, result);
                checkRef(ir, idx, v.rhs, "br phi", false, result);
                break;

            case Op::Return:
                checkRef(ir, idx, v.lhs, "return value", false, result);
                break;

            case Op::LocalSet:
            case Op::GlobalSet:
                checkRef(ir, idx, v.lhs, "lhs", false, result);
                break;

            // ---- 這些opcode不使用lhs/rhs/operands當SSA ref ----
            case Op::Param:
            case Op::I32Const:
            case Op::I64Const:
            case Op::F64Const:
            case Op::LocalGet:
            case Op::LocalTee:
            case Op::GlobalGet:
            case Op::Else:
            case Op::End:
            case Op::Loop:
            case Op::Unreachable:
                break;

            default:
                break;
        }
    }

    return result;
}
