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
        case Op::And:     // 新增
        case Op::Or:      // 新增
        case Op::Xor:     // 新增
        case Op::Shl:     // 新增
        case Op::Shr_S:   // 新增
        case Op::Shr_U:   // 新增
        case Op::Rotl:    // 新增（如果你加了）
        case Op::Rotr:    // 新增（如果你加了）
            std::cout << "(v" << v.lhs << ", v" << v.rhs << ")";
            break;
        case Op::Eqz:
        case Op::Clz:     // 新增
        case Op::Ctz:     // 新增
        case Op::Popcnt:  // 新增
            std::cout << "(v" << v.lhs << ")";
            break;
        case Op::If:
            std::cout << "(v" << v.lhs << ")";  // 显示条件
            break;
        case Op::Else:
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
            std::cout << "(ptr=v" << v.lhs << ")";
            break;
        case Op::Store:
            std::cout << "(ptr=v" << v.lhs << ", val=v" << v.rhs << ")";
            break;
        case Op::F64Load:
            std::cout << "(ptr=v" << v.lhs << ")";
            break;
        case Op::F64Store:
            std::cout << "(ptr=v" << v.lhs << ", val=v" << v.rhs << ")";
            break;
        case Op::Return:
            std::cout << "(v" << v.lhs << ")";
            break;
        }

        std::cout << "\n";
        std::cout.flush();
    }
}
