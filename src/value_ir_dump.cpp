#include "value_ir_dump.hpp"
#include <iostream>

void dumpValueIR(const ValueIR& values) {
    for (auto& v : values) {
        std::cout << "v" << v.id << " = " << opToString(v.op);

        switch (v.op) {
        case Op::Param:
            std::cout << "(param " << v.paramIndex << ")";
            break;
        case Op::Const:
            std::cout << "(" << v.constValue << ")";
            break;
        case Op::Add:
            std::cout << "(v" << v.lhs << ", v" << v.rhs << ")";
            break;
        case Op::Sub:
            std::cout << "(v" << v.lhs << ", v" << v.rhs << ")";
            break;
        case Op::Mul:
            std::cout << "(v" << v.lhs << ", v" << v.rhs << ")";
            break;
        case Op::Return:
            std::cout << "(v" << v.lhs << ")";
            break;
        }

        std::cout << "\n";
    }
}
