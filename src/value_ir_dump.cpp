#include "value_ir_dump.hpp"
#include <iostream>

void dumpValueIR(const ValueIR& values) {
    for (auto& v : values) {
        std::cout << "v" << v.id << " = " << opToString(v.op);

        switch (v.op) {
        case Op::Param:
            std::cout << "(" << v.paramIndex << ")";
            break;
        case Op::I32Const:
            std::cout << "(" << v.constValue << ")";
            break;
        case Op::I64Const:
            std::cout << "(" << v.constValue << ")";
            break;
        case Op::F64Const:
            std::cout << "(" << v.fconst << ")";
            break;

        // ---- 局部變量 / global 存取：原本完全沒印 index，補上 ----
        case Op::LocalGet:
            std::cout << "(local_index=" << v.paramIndex << ")";
            break;
        case Op::LocalSet:
            std::cout << "(local_index=" << v.paramIndex
                       << ", val=v" << v.lhs << ")";
            break;
        case Op::LocalTee:
            std::cout << "(local_index=" << v.paramIndex
                       << ", val=v" << v.lhs << ")";
            break;
        case Op::GlobalGet:
            std::cout << "(global_index=" << v.globalIndex << ")";
            break;
        case Op::GlobalSet:
            std::cout << "(global_index=" << v.globalIndex
                       << ", val=v" << v.lhs << ")";
            break;

        // ---- i32 二元運算 / 比較 / 位元運算 ----
        case Op::Add:
        case Op::Sub:
        case Op::Mul:
        case Op::Div_S:
        case Op::Div_U:
        case Op::Rem_S:
        case Op::Rem_U:
        case Op::Eq:
        case Op::Ne:
        case Op::Lt_S:
        case Op::Lt_U:
        case Op::Gt_S:
        case Op::Gt_U:
        case Op::Le_S:
        case Op::Le_U:
        case Op::Ge_S:
        case Op::Ge_U:
        case Op::And:
        case Op::Or:
        case Op::Xor:
        case Op::Shl:
        case Op::Shr_S:
        case Op::Shr_U:
        case Op::Rotl:
        case Op::Rotr:
            std::cout << "(v" << v.lhs << ", v" << v.rhs << ")";
            break;

        case Op::Eqz:
        case Op::Clz:
        case Op::Ctz:
        case Op::Popcnt:
            std::cout << "(v" << v.lhs << ")";
            break;

        // ---- f64 二元運算 / 比較：原本完全沒印，補上 ----
        case Op::F64Add:
        case Op::F64Sub:
        case Op::F64Mul:
        case Op::F64Div:
        case Op::F64Min:
        case Op::F64Max:
        case Op::F64Eq:
        case Op::F64Ne:
        case Op::F64Lt:
        case Op::F64Gt:
        case Op::F64Le:
        case Op::F64Ge:
        case Op::F64Pow:
            std::cout << "(v" << v.lhs << ", v" << v.rhs << ")";
            break;

        // ---- f64 一元運算 / 轉換：原本完全沒印，補上 ----
        case Op::F64Abs:
        case Op::F64Neg:
        case Op::F64Sqrt:
        case Op::F64Exp:
        case Op::F64Log:
        case Op::F64Sin:
        case Op::F64Cos:
        case Op::F64ConvertI32S:
        case Op::F64ConvertI32U:
        case Op::F64ConvertI64S:
        case Op::F64ConvertI64U:
        case Op::I32TruncF64S:
        case Op::I32TruncF64U:
        case Op::I64TruncF64S:
        case Op::I64TruncF64U:
        case Op::I32WrapI64:
        case Op::I64ExtendI32S:
        case Op::I64ExtendI32U:
            std::cout << "(v" << v.lhs << ")";
            break;

        case Op::If:
            std::cout << "(v" << v.lhs << ")";
            break;
        case Op::Else:
            break;
        case Op::Loop:
            break;
        case Op::End:
            if (v.constValue == 0) std::cout << "(loop)";
            else if (v.constValue == 1) std::cout << "(block)";
            else if (v.constValue == 2) std::cout << "(if)";
            break;

        case Op::Select:
            // Select(condition, false_val, true_val)
            if (v.operands.size() >= 3) {
                std::cout << "(v" << v.operands[0] << ", v"
                         << v.operands[1] << ", v" << v.operands[2] << ")";
            }
            break;

        case Op::Phi:
            std::cout << "(";
            if (!v.operands.empty()) {
                for (size_t i = 0; i < v.operands.size(); i++) {
                    if (i > 0) std::cout << ", ";
                    std::cout << "v" << v.operands[i];
                }
            }
            std::cout << ")";
            break;

        case Op::Br_if:
            std::cout << "(cond: v" << v.lhs
                    << ", target: v" << v.rhs;
            if (v.constValue == 0)
                std::cout << ", kind: loop_back)";
            else
                std::cout << ", kind: block_exit)";
            break;
        case Op::Br:
            if (v.lhs >= 0)
                std::cout << "(target: v" << v.lhs << ", kind: loop_back)";
            else
                std::cout << "(kind: block/if_exit)";
            break;

        case Op::Load:
            std::cout << "(ptr=v" << v.lhs << ", offset=" << v.mem_offset << ")";
            break;
        case Op::Store:
            std::cout << "(ptr=v" << v.lhs
            << ", val=v" << v.rhs
            << ", offset=" << v.mem_offset << ")";
            break;
        case Op::F64Load:
            std::cout << "(ptr=v" << v.lhs
            << ", offset=" << v.mem_offset << ")";
            break;
        case Op::F64Store:
            std::cout << "(ptr=v" << v.lhs
            << ", val=v" << v.rhs
            << ", offset=" << v.mem_offset << ")";
            break;

        // ---- Call：原本完全沒印被呼叫函式與參數，補上 ----
        case Op::Call:
            std::cout << "(callee=" << v.callee_name << ", args=(";
            for (size_t i = 0; i < v.operands.size(); i++) {
                if (i > 0) std::cout << ", ";
                std::cout << "v" << v.operands[i];
            }
            std::cout << "))";
            break;

        case Op::Return:
            std::cout << "(v" << v.lhs << ")";
            break;

        case Op::Unreachable:
            break;

        // ---- memory bulk 操作：原本完全沒印，補上 ----
        case Op::MemorySize:
            break;
        case Op::MemoryFill:
        case Op::MemoryCopy:
            std::cout << "(";
            for (size_t i = 0; i < v.operands.size(); i++) {
                if (i > 0) std::cout << ", ";
                std::cout << "v" << v.operands[i];
            }
            std::cout << ")";
            break;
        case Op::_Count:
            break;  // 哨兵值，不會有實際節點使用這個 op
        }

        std::cout << "\n";
        std::cout.flush();
    }
}